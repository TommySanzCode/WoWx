import csv,importlib.util,struct,sys,tempfile,unittest
from unittest.mock import patch
from pathlib import Path
spec=importlib.util.spec_from_file_location('profiles',Path(__file__).resolve().parents[1]/'tools/profile_telemetry.py')
profile=importlib.util.module_from_spec(spec);spec.loader.exec_module(profile)
class ProfileTests(unittest.TestCase):
    def test_recorder_profile_header_and_values(self):
        sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
        try:import record_telemetry as recorder
        finally:sys.path.pop(0)
        values=[0]*52;values[2]=12;values[16]=1;values[36]=1
        packet=b'WXQ1'+struct.pack('<52I',*values)
        clock=[0]
        def receive(_):
            clock[0]=2
            return packet,('127.0.0.1',39001)
        with tempfile.TemporaryDirectory() as folder:
            output=Path(folder)/'capture.csv'
            with patch.object(sys,'argv',['record_telemetry.py','--seconds','1','--output',str(output)]),patch.object(recorder.socket,'socket') as factory,patch.object(recorder.time,'monotonic',side_effect=lambda:clock[0]):
                factory.return_value.__enter__.return_value.recvfrom.side_effect=receive
                # A companion-only stream must still fail capture acceptance.
                with self.assertRaisesRegex(SystemExit,'No telemetry received'):recorder.main()
            with output.with_suffix('.profiles.csv').open(newline='') as source:
                rows=list(csv.reader(source))
            self.assertEqual(rows,[profile.FIELDS,list(map(str,values))])
            with output.open(newline='') as source:main_header=next(csv.reader(source))
            self.assertEqual(len(recorder.PROFILE_FIELDS),10)
            self.assertTrue(set(recorder.PROFILE_FIELDS)<=set(main_header))
    def test_bounds(self):
        v=[0]*52;v[2]=12;v[3]=19;v[5]=65535;v[10]=8;v[11]=5;v[12]=v[13]=1;v[14]=v[16]=8;v[15]=v[17]=1;v[21]=2;v[29]=8;v[34]=32;v[36]=1
        v[23:26]=[65536,8,65536];v[30:33]=[65536,8,8192];v[37:41]=v[41:45]=[65536,16,65536,3];v[45:48]=[65536,8,65536]
        blob=b'WXQ1'+struct.pack('<52I',*v);self.assertEqual(len(profile.FIELDS),52);self.assertEqual(profile.decode(blob),tuple(v))
        for n in range(len(blob)):self.assertIsNone(profile.decode(blob[:n]))
        self.assertIsNone(profile.decode(blob+b'\0'));self.assertIsNone(profile.decode(b'NOPE'+blob[4:]))
        for i in (2,3,5,10,11,12,13,14,15,16,17,21,23,24,25,29,30,31,32,34,36,37,38,39,40,41,42,43,44,45,46,47):
            bad=v.copy();bad[i]+=1;self.assertIsNone(profile.decode(b'WXQ1'+struct.pack('<52I',*bad)),str(i))
if __name__=='__main__':unittest.main()
