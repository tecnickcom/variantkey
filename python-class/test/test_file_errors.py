"""Tests for variantkey file errors."""

# test_esid.py
# @category   Test
# @author     Nicola Asuni <info@tecnick.com>
# @link       https://github.com/tecnickcom/variantkey
# @license    MIT [LICENSE](https://raw.githubusercontent.com/tecnickcom/variantkey/main/LICENSE)

import pyvariantkey.variantkey as pyvk
from unittest import TestCase


class TestFunctions(TestCase):

    def test_class_file_error_genoref(self):
        with self.assertRaises(Exception):
            pyvk.VariantKey("/wrong/path/error.bin", None, None, None)

    def test_class_file_error_nrvk(self):
        with self.assertRaises(Exception):
            pyvk.VariantKey(None, "/wrong/path/error.bin", None, None)

    def test_class_file_error_rsvk(self):
        with self.assertRaises(Exception):
            pyvk.VariantKey(None, None, "/wrong/path/error.bin", None)

    def test_class_file_error_vkrs(self):
        with self.assertRaises(Exception):
            pyvk.VariantKey(None, None, None, "/wrong/path/error.bin")

    def test_class_file_all_none(self):
        c = pyvk.VariantKey(None, None, None, None)
        self.assertEqual(c.genoref_size, 0)
        self.assertIsNone(c.nrvk_mf)
        self.assertIsNone(c.nrvk_mc)
        self.assertEqual(c.nrvk_nrows, 0)
        self.assertIsNone(c.rsvk_mf)
        self.assertIsNone(c.rsvk_mc)
        self.assertEqual(c.rsvk_nrows, 0)
        self.assertIsNone(c.vkrs_mf)
        self.assertIsNone(c.vkrs_mc)
        self.assertEqual(c.vkrs_nrows, 0)
        c.close()
