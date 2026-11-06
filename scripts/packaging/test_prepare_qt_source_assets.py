#!/usr/bin/env python3
"""Offline tests for Qt source verifier, no downloads."""
import hashlib
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import prepare_qt_source_assets as mod

class SourceAssets(unittest.TestCase):
    def test_offline_exact_hash(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            inputs = {"a.tar.xz": b"test-qtbase", "b.tar.xz": b"test-qtsvg"}
            for name, data in inputs.items():
                (root / name).write_bytes(data)
            hashes = {k: hashlib.sha256(v).hexdigest() for k, v in inputs.items()}
            with patch.dict(mod.ASSETS, hashes, clear=True):
                mod.prepare(root, offline=True)
            self.assertIn("a.tar.xz", (root / "SHA256SUMS.txt").read_text())

    def test_offline_bad_hash_is_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / "a.tar.xz").write_bytes(b"tampered")
            with patch.dict(mod.ASSETS, {"a.tar.xz": "0" * 64}, clear=True):
                with self.assertRaisesRegex(ValueError, "BAD HASH"):
                    mod.prepare(root, offline=True)

    def test_offline_missing_file_is_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            with patch.dict(mod.ASSETS, {"missing.tar.xz": "0" * 64}, clear=True):
                with self.assertRaisesRegex(ValueError, "absent"):
                    mod.prepare(Path(d), offline=True)

if __name__ == "__main__":
    unittest.main()
