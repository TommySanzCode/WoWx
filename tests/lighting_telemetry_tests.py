import importlib.util,struct,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('light',Path(__file__).resolve().parents[1]/'tools/lighting_telemetry.py')
light=importlib.util.module_from_spec(spec);spec.loader.exec_module(light)
class LightingTelemetry(unittest.TestCase):
    def test_packet(self):
        v=[0]*17+[1440.,90.,145.];v[2]=9;v[3]=7;v[4]=2;v[5]=1;v[13]=1;v[14]=2
        encode=lambda values:b'WXL2'+struct.pack('<17I3f',*values)
        data=encode(v);self.assertEqual(len(light.FIELDS),34);self.assertEqual(light.decode(data),tuple(v)+(0,)*13+(2,))
        for n in range(len(data)):self.assertIsNone(light.decode(data[:n]))
        self.assertIsNone(light.decode(data+b'\0'));self.assertIsNone(light.decode(b'BAD!'+data[4:]))
        for field in (2,3,4,5,13,14):
            bad=v.copy();bad[field]=11 if field==2 else bad[field]+1;self.assertIsNone(light.decode(encode(bad)))
        for field in (17,18,19):
            for value in (float('nan'),float('inf'),-float('inf')):
                bad=v.copy();bad[field]=value;self.assertIsNone(light.decode(encode(bad)))
        for field,value in ((17,2880),(18,-641),(18,145),(19,0),(19,161)):
            bad=v.copy();bad[field]=value;self.assertIsNone(light.decode(encode(bad)))
    def test_palette(self):
        v=[0]*17+[1440.,90.,145.]+[255,192]+[0x667788]*8+[0.,0.,1.];v[2]=9
        encode=lambda values:b'WXL3'+struct.pack('<17I3f10I3f',*values)
        data=encode(v);self.assertEqual(len(data),136);self.assertEqual(light.decode(data),tuple(v)+(3,))
        for n in range(len(data)):self.assertIsNone(light.decode(data[:n]))
        self.assertIsNone(light.decode(data+b'\0'))
        for field,value in ((20,256),(21,193),(22,0x1000000),(29,0xffffffff),(30,float('nan')),(31,float('inf')),(32,0.)):
            bad=v.copy();bad[field]=value;self.assertIsNone(light.decode(encode(bad)))
if __name__=='__main__':unittest.main()
