import importlib.util,struct,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('weather',Path(__file__).resolve().parents[1]/'tools/weather_telemetry.py')
weather=importlib.util.module_from_spec(spec);spec.loader.exec_module(weather)
class WeatherTelemetry(unittest.TestCase):
    def test_packet_boundaries(self):
        values=[123,240,10,1,1,8533,0,2,1,2,1,0,0,0,.25,.2]
        encode=lambda v:b'WXW1'+struct.pack('<14I2f',*v)
        data=encode(values);decoded=weather.decode(data);self.assertEqual(len(weather.FIELDS),16);self.assertEqual(decoded[:14],tuple(values[:14]));self.assertAlmostEqual(decoded[15],.2)
        for n in range(len(data)):self.assertIsNone(weather.decode(data[:n]))
        self.assertIsNone(weather.decode(data+b'\0'));self.assertIsNone(weather.decode(b'BAD!'+data[4:]))
        for field,value in ((2,9),(3,10),(4,4),(6,2),(8,2),(10,4),(11,2),(12,3),(14,-.01),(15,1.01),(14,float('nan')),(15,float('inf'))):
            bad=values.copy();bad[field]=value;self.assertIsNone(weather.decode(encode(bad)))
        live=values.copy();live[2]=live[3]=0;self.assertIsNotNone(weather.decode(encode(live)))
if __name__=='__main__':unittest.main()
