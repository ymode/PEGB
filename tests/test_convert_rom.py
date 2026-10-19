#!/usr/bin/env python3
"""Host regression tests for ROM conversion; no Pico SDK or game ROMs needed."""

import argparse
import importlib.util
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tools/convert_rom.py"


def load_converter():
    spec = importlib.util.spec_from_file_location("convert_rom", SOURCE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class ConverterTests(unittest.TestCase):
    def setUp(self):
        self.converter = load_converter()
        self.temp = tempfile.TemporaryDirectory(prefix="pegb-convert-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "tools").mkdir()
        (self.root / "src").mkdir()
        shutil.copyfile(SOURCE, self.root / "tools/convert_rom.py")

    def compile_and_run(self, source, expected):
        c_file = self.root / "check.c"
        c_file.write_text(source, encoding="utf-8")
        binary = self.root / "check"
        compiler = shlex.split(os.environ.get("CC", "cc"))
        # Explicitly enable trigraphs: the firmware uses C11, where they apply.
        compiled = subprocess.run(compiler + ["-std=c11", "-trigraphs", "-Wall", "-Wextra",
                                   "-Werror", "-pedantic", str(c_file),
                                   "-o", str(binary)], capture_output=True)
        self.assertEqual(compiled.returncode, 0, compiled.stderr.decode(errors="replace"))
        result = subprocess.run([str(binary)], check=True, capture_output=True)
        self.assertEqual(result.stdout, expected)

    def test_helper_round_trip(self):
        # Every ASCII control followed by every hex digit catches greedy \\xHH.
        controls = list(range(32)) + [127]
        raw = ''.join(chr(c) + digit for c in controls
                      for digit in "0123456789abcdefABCDEF")
        raw += '\\"\n\r\t'
        escaped = self.converter.escape_c_string(raw)
        self.assertFalse(any(ord(c) < 32 or ord(c) == 127 for c in escaped))
        self.compile_and_run(
            '#include <stdio.h>\nstatic const char value[] = "' + escaped +
            '";\nint main(void) {\n'
            'return fwrite(value, 1, sizeof(value)-1, stdout) == sizeof(value)-1'
            ' ? 0 : 1;\n}\n', raw.encode("ascii"))

    def test_helper_trigraphs(self):
        raw = "??/??=??'??(??)??!??<??>??-"
        escaped = self.converter.escape_c_string(raw)
        self.compile_and_run(
            '#include <stdio.h>\nstatic const char value[] = "' + escaped +
            '";\nint main(void) {\n'
            'return fwrite(value, 1, sizeof(value)-1, stdout) == sizeof(value)-1'
            ' ? 0 : 1;\n}\n', raw.encode("ascii"))

    def test_title_extraction(self):
        def title(raw):
            return self.converter.get_rom_title(bytes(0x134) + raw)
        self.assertEqual(title(b'  GAME  \0IGNORED'), "GAME")
        self.assertEqual(title(b'A\n\r\t\x01\x7f\xffB\0'), "AB")
        self.assertEqual(title(b'\0'), "UNKNOWN")
        self.assertEqual(title(b' ' * 16), "UNKNOWN")
        self.assertEqual(title(b'ABCDEFGHIJKLMNOPextra'), "ABCDEFGHIJKLMNOP")
        self.assertEqual(title(b'"A\\B"\0'), '"A\\B"')

    def convert_and_check(self, cases, directory=False):
        inputs = self.root / "roms"
        inputs.mkdir()
        expected = []
        paths = []
        for filename, raw_title, title in cases:
            data = bytearray((i * 37 + 11) & 255 for i in range(0x150))
            data[0x134:0x144] = raw_title.ljust(16, b'\0')[:16]
            path = inputs / filename
            try:
                path.write_bytes(data)
            except (OSError, ValueError) as exc:
                self.skipTest(f"filesystem cannot represent {filename!r}: {exc}")
            paths.append(path)
            # The C probe emits title length/title then ROM size/data.
            expected.append(bytes([len(title)]) + title.encode("ascii") +
                            len(data).to_bytes(4, "little") + data)
        args = [str(inputs)] if directory else [str(p) for p in paths + paths[:1]]
        subprocess.run([sys.executable, str(self.root / "tools/convert_rom.py")]
                       + args, check=True, capture_output=True)
        header = self.root / "src/rom_data.h"
        self.assertEqual(header.read_text().count("// "), len(cases) + 1)
        self.compile_and_run('''#include <stdio.h>
#include <string.h>
#include "src/rom_data.h"
int main(void) {
    for (size_t i = 0; i < ROM_COUNT; ++i) {
        const rom_entry_t *rom = &rom_list[i];
        putchar((int)strlen(rom->title));
        if (fwrite(rom->title, 1, strlen(rom->title), stdout) != strlen(rom->title))
            return 1;
        for (unsigned j = 0; j < 4; ++j)
            putchar((int)((rom->size >> (8 * j)) & 255));
        if (fwrite(rom->data, 1, rom->size, stdout) != rom->size)
            return 1;
    }
    return 0;
}
''', b''.join(expected))

    def test_header_titles(self):
        self.convert_and_check([
            ("ordinary.gb", b"TETRIS", "TETRIS"),
            ("quotes.gb", b'GAME "A"', 'GAME "A"'),
            ("slashes.gb", b'A\\n\\x41\\', 'A\\n\\x41\\'),
            ("injection.gb", b'"}; int evil; //', '"}; int evil; //'),
            ("filtered.gb", b'A\n\r\t\x01\x7f\xffB', 'AB'),
            ("unknown.gb", b'\0', 'UNKNOWN'),
        ])

    def test_header_trigraphs(self):
        self.convert_and_check([("trigraphs.gb", b'??/??=??!', '??/??=??!')])

    def test_unusual_filenames(self):
        # Newlines must not escape the generated // comment. POSIX filenames
        # cannot contain slash (the final character of the ??/ trigraph).
        for filename in ['quote"back\\.gb', 'line\n#error injected\n.gb',
                         'control\x01f\x7f\r\t.gb', "trigraph??=??!??(??)??'??<??>??-.gb",
                         'unicode-雪.gb']:
            with self.subTest(filename=repr(filename)):
                # Separate directories permit each case to use the same helper.
                with tempfile.TemporaryDirectory(dir=self.root) as temp:
                    old_root = self.root
                    self.root = Path(temp)
                    (self.root / "tools").mkdir()
                    (self.root / "src").mkdir()
                    shutil.copyfile(SOURCE, self.root / "tools/convert_rom.py")
                    try:
                        self.convert_and_check([(filename, b'NORMAL', 'NORMAL')])
                    finally:
                        self.root = old_root

    def test_directory_and_deduplication(self):
        self.convert_and_check([("a.gb", b'FIRST', 'FIRST'),
                                ("b.GB", b'SECOND', 'SECOND')], directory=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=SOURCE,
                        help="converter implementation to test")
    args, remaining = parser.parse_known_args()
    SOURCE = args.source.resolve()
    unittest.main(argv=[sys.argv[0]] + remaining)
