import importlib.util,struct,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('draw',Path(__file__).resolve().parents[1]/'tools/draw_telemetry.py')
draw=importlib.util.module_from_spec(spec);spec.loader.exec_module(draw)
class DrawTests(unittest.TestCase):
    def packet(self,**changes):
        v=dict(zip(draw.FIELDS,[100,3,11,1,14,2,1,4,1,2,1539,1536,8,8,3,2,1]));v.update(changes)
        return b'WXD1'+struct.pack('<17I',*(v[k] for k in draw.FIELDS))
    def test_wire(self):
        p=self.packet();self.assertEqual(len(draw.FIELDS),17);self.assertIsNotNone(draw.decode(p))
        for n in range(len(p)):self.assertIsNone(draw.decode(p[:n]))
        self.assertIsNone(draw.decode(p+b'\0'));self.assertIsNone(draw.decode(b'WXD2'+p[4:]))
    def test_accounting(self):
        for bad in ({'scope':2},{'fixture':5},{'world_ms':9},{'flush_wait_ms':14},{'flushes':0},
                    {'indices':1540},{'largest_batch':1539},{'largest_batch':0},{'batches':0},
                    {'batches':1},{'legacy_batches':1},{'legacy_batches':514}):
            with self.subTest(bad=bad):self.assertIsNone(draw.decode(self.packet(**bad)))
    def test_empty_and_live(self):
        self.assertIsNotNone(draw.decode(self.packet(batches=0,indices=0,largest_batch=0,legacy_batches=0)))
        self.assertIsNotNone(draw.decode(self.packet(fixture=0,scope=2)))
        self.assertIsNone(draw.decode(self.packet(fixture=0,scope=1)))
if __name__=='__main__':unittest.main()
