import importlib.util,struct,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('world_index',Path(__file__).resolve().parents[1]/'tools/world_index.py')
index=importlib.util.module_from_spec(spec);spec.loader.exec_module(index)
class WorldIndexTests(unittest.TestCase):
    def test_roundtrip(self):
        rows=[dict(map=0,x=31,y=32,kind=0,file='M0003132.WXP',bytes=500),
              dict(map=34,x=0,y=0,kind=1,file='G034.WXP',bytes=1000)]
        blob=index.encode(rows);self.assertEqual(index.decode(blob),rows);self.assertEqual(blob[4],2)
        self.assertEqual(index.encode(rows[:1])[4],1)
        for size in range(len(blob)):
            with self.assertRaises(ValueError):index.decode(blob[:size])
        for at,value in [(4,1),(4,3),(8,0),(8,8193),(12,31),(48,1000),(68,32),(76,2)]:
            bad=bytearray(blob);struct.pack_into('<I',bad,at,value)
            with self.assertRaises(ValueError):index.decode(bad)
        for bad in [rows[::-1],rows+rows[-1:],rows+[dict(map=34,x=1,y=1,kind=0,file='M0340101.WXP',bytes=1000)]]:
            with self.assertRaises(ValueError):index.encode(bad)
        for args in [(34,1,0,1),(34,0,0,2),(1000,0,0,1),(0,64,0,0)]:
            with self.assertRaises(ValueError):index.filename(*args)
if __name__=='__main__':unittest.main()
