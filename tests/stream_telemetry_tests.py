import importlib.util,struct,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('stream',Path(__file__).resolve().parents[1]/'tools/stream_telemetry.py')
stream=importlib.util.module_from_spec(spec);spec.loader.exec_module(stream)
class StreamTests(unittest.TestCase):
    def test_full_coverage(self):
        main=[{'time_ms':i*33,'replay_frame':i} for i in range(10)]
        aux=[{'time_ms':i*33,'frame':i} for i in range(10)]
        self.assertTrue(stream.full_coverage(main,aux)['passed'])
        self.assertTrue(stream.full_coverage(main,aux[:-1])['passed'])
        for i in range(9):self.assertFalse(stream.full_coverage(main,aux[:i]+aux[i+1:])['passed'])
        self.assertFalse(stream.full_coverage(main,aux+aux[:1])['passed'])
        self.assertFalse(stream.full_coverage(main+main[:1],aux)['passed'])
        self.assertFalse(stream.full_coverage(main,aux+[{'time_ms':330,'frame':10}])['passed'])
        aux[3]['frame']=30;self.assertFalse(stream.full_coverage(main,aux)['passed'])
        self.assertFalse(stream.full_coverage([],[])['passed'])
    def test_exact(self):
        v=[0]*31;v[2]=7;v[3]=8;v[5]=1;v[7]=15;v[22]=2;v[23]=1
        blob=b'WXS1'+struct.pack('<31I',*v)
        self.assertEqual(len(stream.FIELDS),75);self.assertEqual(stream.decode(blob),tuple(v)+(-1,)*44)
        for n in range(len(blob)):self.assertIsNone(stream.decode(blob[:n]))
        self.assertIsNone(stream.decode(blob+b'\0'));self.assertIsNone(stream.decode(b'BAD!'+blob[4:]))
        for field in (2,3,5,7,22,23):
            bad=v.copy();bad[field]=9 if field==2 else bad[field]+1;self.assertIsNone(stream.decode(b'WXS1'+struct.pack('<31I',*bad)))
    def test_frame_budget(self):
        v=[0]*52;v[2]=8;v[31]=1;v[32:36]=[65536,16,65536,3]
        v[36:40]=[8192,2,16384,0];v[40:44]=[24576,6,16384,1];v[44:48]=v[48:52]=[16384,4,16384,1]
        blob=b'WXS2'+struct.pack('<52I',*v);self.assertEqual(stream.decode(blob),tuple(v)+(-1,)*23)
        for n in range(len(blob)):self.assertIsNone(stream.decode(blob[:n]))
        self.assertIsNone(stream.decode(blob+b'\0'));self.assertIsNone(stream.decode(b'WXS1'+blob[4:]))
        for field in (31,32,33,34,35,36,41,46,51):
            bad=v.copy();bad[field]+=1;self.assertIsNone(stream.decode(b'WXS2'+struct.pack('<52I',*bad)))
        v[31]=0;self.assertIsNone(stream.decode(b'WXS2'+struct.pack('<52I',*v)))
    def test_global_maps(self):
        v=[0]*52;v[2]=13;v[3]=31;v[31]=1
        self.assertEqual(stream.decode(b'WXS2'+struct.pack('<52I',*v)),tuple(v)+(-1,)*23)
        v[3]=32;self.assertIsNone(stream.decode(b'WXS2'+struct.pack('<52I',*v)))
        v[3]=0;v[2]=14;self.assertIsNone(stream.decode(b'WXS2'+struct.pack('<52I',*v)))
    def test_clock_budget(self):
        v=[0]*62;v[31]=1;v[52:]=[1,5,2,6,2,2,3,5,2,1]
        data=b'WXS3'+struct.pack('<62I',*v)
        self.assertEqual(stream.decode(data),tuple(v)+(-1,)*13)
        for n in range(len(data)):self.assertIsNone(stream.decode(data[:n]))
        self.assertIsNone(stream.decode(data+b'\0'))
        self.assertIsNone(stream.decode(b'WXS2'+data[4:]))
        for field,value in ((31,0),(52,0),(52,2),(53,16),(53,4),(54,3),(55,13),(59,6),(58,1)):
            bad=v.copy();bad[field]=value;self.assertIsNone(stream.decode(b'WXS3'+struct.pack('<62I',*bad)))
        # A blocking operation may exceed its slice; record the overrun rather
        # than dropping evidence. A disabled clock has explicit zero metrics.
        v[58]=500;self.assertEqual(stream.decode(b'WXS3'+struct.pack('<62I',*v)),tuple(v)+(-1,)*13)
        v[52:]=[0]*10;self.assertEqual(stream.decode(b'WXS3'+struct.pack('<62I',*v)),tuple(v)+(-1,)*13)
        v[52:]=[1,0,0,12,0,0,0,5,0,0];self.assertEqual(stream.decode(b'WXS3'+struct.pack('<62I',*v)),tuple(v)+(-1,)*13)
        v[52:]=[1,0,0,0,0,0,0,0,0,0];self.assertEqual(stream.decode(b'WXS3'+struct.pack('<62I',*v)),tuple(v)+(-1,)*13)
    def test_index_publication(self):
        v=[0]*67;v[31]=1;v[22]=4;v[27]=1048576;v[62:]=[262144,786432,262144,786432,2]
        data=b'WXS4'+struct.pack('<67I',*v);self.assertEqual(stream.decode(data),tuple(v)+(-1,)*8)
        for n in range(len(data)):self.assertIsNone(stream.decode(data[:n]))
        self.assertIsNone(stream.decode(data+b'\0'));self.assertIsNone(stream.decode(b'WXS3'+data[4:]))
        for field,value in ((22,6),(22,3),(22,5),(27,786431),(31,0),(62,262208),(63,8388672),(64,786496),(64,1),(65,0),(65,1)):
            bad=v.copy();bad[field]=value;self.assertIsNone(stream.decode(b'WXS4'+struct.pack('<67I',*bad)))
        v[22]=5;v[64]=v[63];self.assertEqual(stream.decode(b'WXS4'+struct.pack('<67I',*v)),tuple(v)+(-1,)*8)
        v[22]=0;v[27]=0;v[62]=v[65]=0;self.assertEqual(stream.decode(b'WXS4'+struct.pack('<67I',*v)),tuple(v)+(-1,)*8)
    def test_selection(self):
        v=[0]*75;v[31]=1;v[67:]=[2048,1,4096,10000,2,1,15,15]
        blob=b'WXS5'+struct.pack('<75I',*v);self.assertEqual(stream.decode(blob),tuple(v))
        for n in range(len(blob)):self.assertIsNone(stream.decode(blob[:n]))
        self.assertIsNone(stream.decode(blob+b'\0'))
        for field,value in ((31,0),(5,1),(67,2049),(68,2),(69,10001),(70,131073),(73,14)):
            bad=v.copy();bad[field]=value;self.assertIsNone(stream.decode(b'WXS5'+struct.pack('<75I',*bad)))
    def test_actors(self):
        v=[0]*40;v[2]=8;v[3]=255;v[4]=v[5]=v[6]=32;v[7]=1;v[39]=15
        data=b'WXN1'+struct.pack('<40I',*v)
        self.assertEqual(len(stream.ACTOR_FIELDS),58);self.assertEqual(stream.decode_actors(data),tuple(v)+(-1,)*18)
        for n in range(len(data)):self.assertIsNone(stream.decode_actors(data[:n]))
        self.assertIsNone(stream.decode_actors(data+b'\0'));self.assertIsNone(stream.decode_actors(b'BAD!'+data[4:]))
        for field in (2,3,4,5,6,7,39):
            bad=v.copy();bad[field]+=1;self.assertIsNone(stream.decode_actors(b'WXN1'+struct.pack('<40I',*bad)))
    def test_scheduled_actors(self):
        v=[0]*48;v[2]=8;v[40:44]=[12,5,720,2];v[44:48]=[18,0,1200,1]
        data=b'WXN2'+struct.pack('<48I',*v);self.assertEqual(stream.decode_actors(data),tuple(v)+(-1,)*10)
        for n in range(len(data)):self.assertIsNone(stream.decode_actors(data[:n]))
        self.assertIsNone(stream.decode_actors(data+b'\0'))
        self.assertIsNone(stream.decode_actors(b'WXN1'+data[4:]))
        for field,value in ((40,321),(41,309),(43,13),(44,0),(47,19)):
            bad=v.copy();bad[field]=value;self.assertIsNone(stream.decode_actors(b'WXN2'+struct.pack('<48I',*bad)))
    def test_composition(self):
        v=[0]*58;v[2]=11;v[48:58]=[7,65536,8,8192,19,5,0,0x12345678,5,80]
        data=b'WXN3'+struct.pack('<58I',*v);self.assertEqual(stream.decode_actors(data),tuple(v))
        for n in range(len(data)):self.assertIsNone(stream.decode_actors(data[:n]))
        self.assertIsNone(stream.decode_actors(data+b'\0'))
        self.assertIsNone(stream.decode_actors(b'WXN2'+data[4:]))
        for field in (48,49,50,51,57):
            bad=v.copy();bad[field]=9 if field==48 else bad[field]+1
            self.assertIsNone(stream.decode_actors(b'WXN3'+struct.pack('<58I',*bad)))
if __name__=='__main__':unittest.main()
