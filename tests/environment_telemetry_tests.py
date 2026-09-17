import sys,struct,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from environment_telemetry import FIELDS,decode
class EnvironmentTelemetry(unittest.TestCase):
    def test_wire(self):
        v=[123,456,13,3,1,2,1,2,1,42,1,77,0,8192,0,0,1,1,.6,1.,2.,3.]
        enc=lambda r:struct.pack('<4s18I4f',b'WXY1',*r)
        data=enc(v);self.assertEqual(len(FIELDS),22);self.assertEqual(decode(data)[:18],tuple(v[:18]))
        for n in range(len(data)):self.assertIsNone(decode(data[:n]))
        self.assertIsNone(decode(data+b'0'));self.assertIsNone(decode(b'BAD!'+data[4:]))
        for at,x in [(2,12),(3,32),(5,3),(7,3),(8,2),(9,8193),(12,4),(13,8388609),(14,5),(17,2),(18,-1),(19,float('nan'))]:
            bad=v.copy();bad[at]=x;self.assertIsNone(decode(enc(bad)))
    def test_normal_unknown(self):
        v=[1,2,0,0,0,0xffffffff,0xffffffff,0,0,0,0,0,0xffffffff,0,0,0,0,1,0.,1.,2.,3.]
        self.assertEqual(decode(struct.pack('<4s18I4f',b'WXY1',*v)),tuple(v))
if __name__=='__main__':unittest.main()
