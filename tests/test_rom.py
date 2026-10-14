import unittest
from tools.verify_rom import inspect

class RomIntakeTests(unittest.TestCase):
    def image(self):
        b = bytearray(2097152)
        b[0x7fc0:0x7fd5] = b"YOSHI'S ISLAND".ljust(21,b' ')
        b[0x7fdc:0x7fe0] = bytes([0xff,0xff,0,0])
        return bytes(b)
    def test_copier_header_normalization(self):
        b=self.image()
        a,c=inspect(b),inspect(bytes(512)+b)
        self.assertEqual(a['normalized_sha256'],c['normalized_sha256'])
        self.assertEqual(c['copier_header_bytes'],512)
        self.assertNotEqual(a['file_sha256'],c['file_sha256'])
    def test_reject_wrong_title(self):
        with self.assertRaises(ValueError): inspect(bytes(2097152))
    def test_reject_truncated(self):
        with self.assertRaises(ValueError): inspect(bytes(100))

if __name__=='__main__': unittest.main()
