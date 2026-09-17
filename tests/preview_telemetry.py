"""Synthetic parser/checker tests; never evidence of an emulator run."""
import copy
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from record_telemetry import FIELDS,decode_packet
from preview_validation import customization_checks
from action_telemetry import FIELDS as ACTION_FIELDS,decode as decode_actions

class Telemetry(unittest.TestCase):
    def test_action_companion_packet(self):
        data=bytearray(264);data[:4]=b'WXF1';struct.pack_into('<I',data,6*4,2);struct.pack_into('<I',data,11*4,1)
        row=decode_actions(data);self.assertEqual(len(row),len(ACTION_FIELDS));self.assertEqual(dict(zip(ACTION_FIELDS,row))['cast_phase'],2)
        self.assertIsNone(decode_packet(data))
        for size in (0,4,260,263,265):self.assertIsNone(decode_actions(data[:size] if size<264 else data+b'\0'))
        data[6*4]=6;self.assertIsNone(decode_actions(data))
    def test_version_history_and_signed_fields(self):
        lengths=(96,132,144,176,192,216,236,264,524,528,568,612,616,640,680,692,760,784,824,856,912,984,1000,1016,1060,1092,1124,1176,1196,1220,1288,1328,1368)
        versions='123456789ABCDEFGHIJKLMNOPQRSTUVWX'
        self.assertEqual(len(lengths),len(versions))
        for size,version in zip(lengths,versions):
            with self.subTest(version=version):
                data=bytearray(size);data[:4]=('WXT'+version).encode()
                struct.pack_into('<ii',data,17*4,-1,-1)
                row=decode_packet(data);self.assertEqual(len(row),len(FIELDS))
                values=dict(zip(FIELDS,row));self.assertEqual(values['action'],-1);self.assertEqual(values['slot'],-1)
                self.assertEqual(values['pad_connected'],-1 if size<616 else 0)
                self.assertEqual(values['character_randomizations'],0)
                data[:4]=b'BAD!';self.assertIsNone(decode_packet(data))

    def test_new_fields_and_malformed(self):
        data=bytearray(1220);data[:4]=b'WXTU'
        values=(0x04030201,5,42,0xfe123456,4,16)
        struct.pack_into('<6I',data,299*4,*values)
        row=decode_packet(data);self.assertEqual(tuple(row[298:304]),values)
        for size in (0,4,95,1219,1221,2048):self.assertIsNone(decode_packet((data+bytearray(2048))[:size]))
        for index in (9,23,29):
            bad=bytearray(data);struct.pack_into('<f',bad,index*4,float('nan'));self.assertIsNone(decode_packet(bad))
        data[:4]=b'WXTT';self.assertIsNone(decode_packet(data))

    def test_hud_fields(self):
        data=bytearray(1288);data[:4]=b'WXTV'
        hud=(1,117,79,79,1,355,1000,123,0,32,40,0,70,110,768,900,1)
        struct.pack_into('<17I',data,305*4,*hud)
        row=decode_packet(data);self.assertEqual(tuple(row[304:321]),hud)
        values=dict(zip(FIELDS,row));self.assertEqual(values['hud_player_power'],355)
        self.assertEqual(values['hud_target_present'],1)
        self.assertIsNone(decode_packet(data[:-1]))

    def test_portrait_fields(self):
        data=bytearray(1328);data[:4]=b'WXTW'
        metrics=(1,108,0,0x80000002,14,328,2,74,1,1)
        struct.pack_into('<10I',data,322*4,*metrics)
        values=dict(zip(FIELDS,decode_packet(data)))
        self.assertEqual(values['portrait_player_key'],0x80000002)
        self.assertEqual(values['portrait_target_key'],328)
        self.assertEqual(values['portrait_fixture'],1)
        self.assertEqual(values['world_active'],0)
        self.assertIsNone(decode_packet(data[:-1]))

    def test_icon_fields(self):
        data=bytearray(1368);data[:4]=b'WXTX'
        metrics=(1,51962,2524,677840,22,0,0,8,17,1)
        struct.pack_into('<10I',data,332*4,*metrics)
        values=dict(zip(FIELDS,decode_packet(data)))
        self.assertEqual(values['icons_drawn'],8)
        self.assertEqual(values['ui_batches'],17)
        self.assertIsNone(decode_packet(data[:-1]))

    def test_cooldown_fields(self):
        data=bytearray(1440);data[:4]=b'WXTY'
        struct.pack_into('<18I',data,342*4,1,23000,460000,6,6,0,0,45076,*range(8),16,3)
        values=dict(zip(FIELDS,decode_packet(data)))
        self.assertEqual(values['cooldown_remaining_7'],7)
        self.assertEqual(values['cooldown_held_mask'],16)
        self.assertEqual(values['cooldown_fixture_step'],3)
        self.assertIsNone(decode_packet(data[:-1]))

    def test_global_cooldown_fields(self):
        data=bytearray(1472);data[:4]=b'WXTZ'
        struct.pack_into('<8I',data,360*4,2,4,0,7,1,255,1000,3)
        values=dict(zip(FIELDS,decode_packet(data)))
        self.assertEqual(values['gcd_mask'],255)
        self.assertEqual(values['modifier_updates'],4)
        self.assertEqual(values['spellmod_family'],3)
        self.assertIsNone(decode_packet(data[:-1]))

class Customization(unittest.TestCase):
    def setUp(self):
        self.plan={'version':1,'samples':[]};self.rows=[]
        for race in range(1,9):
            for sex in range(2):
                identity=race|(sex<<8);look=0
                for field in range(5):
                    if field<4:look|=1<<(field*8)
                    frame=1000+((race-1)*2+sex)*500+field*64
                    sample=dict(identity=identity,row=field,look=look,facial=int(field==4),frame=frame)
                    self.plan['samples'].append(sample)
                    row=dict(character_screen='6',preview_identity=str(identity),preview_class='1',
                             preview_look=str(look),preview_facial=str(sample['facial']),character_appearance_row=str(field),
                             preview_compositions=str(field+1),preview_atlas_hash=str(100+field),
                             character_randomizations=str((race-1)*2+sex))
                    for offset in range(3):self.rows.append(dict(row,replay_frame=str(frame+offset)))
                self.rows.append(dict(row,replay_frame=str(frame+64),preview_look='33686018',preview_facial='2',
                                      preview_atlas_hash='999',preview_compositions='6',character_randomizations=str((race-1)*2+sex+1)))

    def check(self,rows=None,plan=None):
        checks,_=customization_checks(self.rows if rows is None else rows,self.plan if plan is None else plan)
        return all(checks.values())

    def test_complete(self):self.assertTrue(self.check())

    def test_stale_or_missing_render(self):
        for key,value in (('preview_look','0'),('preview_compositions','0'),('preview_atlas_hash','0'),
                          ('character_appearance_row','4'),('replay_frame','0'),('preview_identity','8')):
            with self.subTest(key=key):
                rows=copy.deepcopy(self.rows)
                for row in rows[:3]:row[key]=value
                self.assertFalse(self.check(rows))
        self.assertFalse(self.check(self.rows[1:]))

    def test_randomize_must_recompose(self):
        for row in self.rows:
            if row['preview_compositions']=='6':row['preview_compositions']='5'
        self.assertFalse(self.check())

    def test_unstable_texture(self):
        self.rows[1]['preview_atlas_hash']='123';self.assertFalse(self.check())

    def test_incomplete_or_duplicate_plan(self):
        self.assertFalse(self.check(plan={'version':1,'samples':self.plan['samples'][:-1]}))
        self.plan['samples'][-1]=self.plan['samples'][0];self.assertFalse(self.check())

if __name__=='__main__':unittest.main()
