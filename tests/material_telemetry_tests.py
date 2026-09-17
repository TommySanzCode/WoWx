import sys,struct,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from material_telemetry import FIELDS,decode
from material_probe import pack
class Materials(unittest.TestCase):
    def test_wire(self):
        row=[123,456,13,3,2,2,2,2,2,2,0x3f82,1]
        data=struct.pack('<4s12I',b'WXM1',*row)
        self.assertEqual(tuple(row)+(-1,)*5,decode(data));self.assertEqual(len(FIELDS),len(row)+5)
        self.assertEqual(tuple(row)+(14,123)+(-1,)*3,decode(struct.pack('<4s14I',b'WXM2',*row,14,123)))
        self.assertEqual(tuple(row)+(14,123,2,2,1<<29),decode(struct.pack('<4s17I',b'WXM3',*row,14,123,2,2,1<<29)))
        for liquid,sequence,frames in ((3,2,1),(2,16,1),(1,1,0),(0,0,1)):
            self.assertIsNone(decode(struct.pack('<4s17I',b'WXM3',*row,14,123,liquid,sequence,frames)))
        self.assertIsNone(decode(struct.pack('<4s14I',b'WXM2',*row,16,123)))
        for bad in (data[:-1],data+b'0',b'BAD!'+data[4:]):self.assertIsNone(decode(bad))
        for at,value in ((2,14),(3,321),(10,4),(11,2)):
            other=row.copy();other[at]=value;self.assertIsNone(decode(struct.pack('<4s12I',b'WXM1',*other)))
    def test_probe(self):
        blob=pack();magic,version,count,stride,x,y,z,size=struct.unpack_from('<4sIII3fI',blob)
        self.assertEqual((magic,version,count,stride,x,y,z,size),(b'WXP1',6,16,64,0,0,0,len(blob)))
        flags=[struct.unpack_from('<9I4f3I',blob,32+i*64)[13] for i in range(count)]
        modes=[1 if f&2 else (f>>7)&7 for f in flags[1:-1]]
        self.assertEqual(modes,list(range(7))*2)
if __name__=='__main__':unittest.main()
