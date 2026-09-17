"""Create a fixed input trace; this does not emulate USB or verify a physical pad."""
import argparse
import struct
import math
import re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,help='Fixture folder; defaults to build/xbox');p.add_argument('--preview',action='store_true');p.add_argument('--soak',action='store_true');p.add_argument('--soak-seconds',type=int);p.add_argument('--corpse',action='store_true');p.add_argument('--map',action='store_true');p.add_argument('--legacy-text',action='store_true');p.add_argument('--test-character');p.add_argument('--walk',action='store_true');p.add_argument('--quest-loop',action='store_true');p.add_argument('--utility',action='store_true');p.add_argument('--actionbar',action='store_true');p.add_argument('--avatar-switch',action='store_true');p.add_argument('--enabled',action='store_true');p.add_argument('--scene',action='store_true');p.add_argument('--quest',action='store_true');p.add_argument('--combat',action='store_true');p.add_argument('--turnin',action='store_true');p.add_argument('--inventory',action='store_true');p.add_argument('--kobolds',action='store_true');p.add_argument('--boundary',action='store_true');p.add_argument('--kobold-turnin',action='store_true');p.add_argument('--hearthstone',action='store_true');p.add_argument('--death',action='store_true');p.add_argument('--vendor',action='store_true');p.add_argument('--appearance',action='store_true');p.add_argument('--camera',action='store_true');p.add_argument('--character',action='store_true');p.add_argument('--character-screens',action='store_true');p.add_argument('--starter-quest',action='store_true');p.add_argument('--camp-quest',action='store_true');p.add_argument('--spellbook',action='store_true');p.add_argument('--spellbook-screens',action='store_true');p.add_argument('--camp-return',type=Path);p.add_argument('--auto-login',action='store_true');p.add_argument('--login-replay',action='store_true');args=p.parse_args()
if sum((args.preview,args.login_replay,args.soak,args.corpse,args.map,args.walk,args.quest_loop,args.utility,args.actionbar,args.avatar_switch,args.enabled,args.scene,args.quest,args.combat,args.turnin,args.inventory,args.kobolds,args.boundary,args.kobold_turnin,args.hearthstone,args.death,args.vendor,args.appearance,args.camera,args.character,args.character_screens,args.starter_quest,args.camp_quest,args.spellbook,args.spellbook_screens))>1:p.error('Select one replay scenario')
root=Path(__file__).resolve().parents[1];records=[];frame=0
destination=args.output or root/'build/xbox'
if args.auto_login and args.login_replay:p.error('Auto-login and login replay are mutually exclusive')
legacy=any(value for key,value in vars(args).items() if key not in ('legacy_text','login_replay','auto_login','output'))
login=(b'WXLT' if args.login_replay or args.preview else b'NONE' if legacy or args.auto_login else b'MENU')
if args.soak_seconds is not None and (not args.soak or not 60<=args.soak_seconds<=3600):p.error('Soak seconds requires --soak and 60-3600 seconds')
soak=(struct.pack('<4sI',b'WXS1',args.soak_seconds or 3600) if args.soak else b'NONE')
font=(b'WXFL' if args.legacy_text else b'NONE')
if args.test_character and (not re.fullmatch(r'[A-Za-z]{2,12}',args.test_character) or not any((args.soak,args.corpse,args.quest_loop,args.walk,args.camp_quest,args.starter_quest,args.character,args.avatar_switch))):p.error('Test character requires a character/journey scenario and 2-12 ASCII letters')
test_character=(b'WXCN'+args.test_character.encode().ljust(13,b'\0') if args.test_character else b'NONE')
route=b'NONE'
if args.camp_return:
    if not args.camp_quest:p.error('--camp-return requires --camp-quest')
    points=[list(map(float,line.split())) for line in args.camp_return.read_text().splitlines() if line.strip() and not line.startswith('#')]
    if not 2<=len(points)<=128 or any(len(q)!=3 or any(not math.isfinite(x) or abs(x)>20000 for x in q) for q in points):p.error('Invalid bounded return route')
    route=struct.pack('<4sI',b'WXRP',len(points))+b''.join(struct.pack('<3f',*q) for q in points)

def step(frames=2,buttons=0,axes=(0,0,0,0,0,0)):
    global frame
    frame+=frames;records.append(struct.pack('<I6hII',frame,*axes,buttons,1))
def press(button):step(buttons=1<<button);step()
if args.preview:
    # Combined login/title/roster/outfits and all five appearance controls.
    # All drafts, including randomized looks, are cancelled without server writes.
    # The login fixture finishes after 600 world frames before raw inputs begin.
    step(900);press(8);press(6);step(30);press(3);step(750)
    step(45,axes=(0,0,18000,0,0,0));step(90);press(12);step(180);press(12);step(360)
    press(11);press(11);step(60);press(2);step(360);press(12) # Race row, male Warrior.
    masks=(0,0x336,0x29a,0x3e,0x83a,0x332,0x88a,0x312,0x1ba)
    for race in range(1,9):
        if race>1:press(14)
        for sex in range(2):
            step(180);press(12) # Class row, allowing the scene/profile to stream.
            for cl in range(1,12):
                if masks[race]&(1<<cl):step(48);press(14)
            press(3) # Appearance; remains on Class when returning.
            for field in range(5):
                press(14);step(60)
                if field<4:press(12)
            press(2);step(90);press(1) # Randomize, then return to the form.
            press(12);press(14);press(11);press(11) # Toggle sex; finish back at Race.
    step(45,axes=(0,0,-15000,-18000,0,0));step(90)
    press(11);press(0);step(300);press(1);press(1);step(180);press(0);step(450)
    press(3);step(120);press(3);step(120);press(1);step(450) # Read-only HUD targets, then clear.
elif args.map:
    step(600);press(8);press(4);step(600)  # Current zone and explored patches.
    press(0);step(180);step(90,axes=(32767,-32767,20000,20000,32767,32767));step(90)
    press(2);step(90);press(11);step(450)  # Center, then continent.
    press(14);step(90);press(13);step(90);press(12);step(180)
    # Closing frame and held inputs must not escape to gameplay.
    step(2,buttons=1<<1,axes=(0,-32767,20000,0,32767,32767))
    step(45,buttons=1<<2,axes=(0,-32767,20000,0,32767,32767));step(90)
    press(4);step(90);press(4);step(90)
    # Preserve menu Back logout and saved state after reconnect.
    press(6);step(30);press(4);step(750);press(4);step(450);press(6);step(90)
elif args.utility:
    step(600);press(8);press(10);step(90);press(1)
    black=1<<10
    step(600,buttons=black)
    step(2,buttons=black|(1<<14));step(90,buttons=black);step();step(120);press(1)
    for direction in (12,11,13):
        step(20,buttons=black);step(2,buttons=black|(1<<direction));step(60,buttons=black);step();step(120);press(1)
    # Confirm with modifier+A, keep holding Black, then send modifier+X.
    # Both action edges and both sticks must be consumed by the utility UI.
    axes=(0,-32767,25000,0,32767,32767)
    step(20,buttons=black);step(30,buttons=black,axes=axes)
    step(2,buttons=black|1,axes=axes);step(30,buttons=black|(1<<2),axes=axes)
    step();step(90);press(1)
    step(20,buttons=black);step(2,buttons=black|(1<<1));step(30,buttons=black);step();step(90)
elif args.actionbar:
    # Read-only layer inspection; no action buttons while a trigger is held.
    step(600);press(8)
    for layer in (1,2,3):
        step(450,axes=(0,0,0,0,32767 if layer&1 else 0,32767 if layer&2 else 0));step(60)
    press(6);step(180,axes=(0,0,0,0,32767,0));step(30);press(6)
    press(10);step(180,axes=(0,0,0,0,0,32767));step(30);press(10)
    step(120)
elif args.spellbook_screens:
    # Read-only UI inspection: show the learned list, then close it.
    step(600);press(6);step(30);press(9);step(1200);press(1);step(90)
elif args.character_screens:
    # Paced visual inspection only: cancel the empty draft, then re-enter the original.
    step(90);press(6);step(30);press(3);step(2100)  # Logout/roster and screenshot.
    press(2);step(900)  # Creation form.
    press(0);step(1200)  # Name keyboard.
    press(1);step(60);press(1);step(90);press(0);step(900)
elif args.hearthstone:
    # Xboxer's unmodified starter bag: food first, hearthstone second. The
    # state-driven gameplay scenarios cover selection without fixed slots.
    step(450);press(10);step(180);press(12);step(120);press(2);step(90);press(1);step(750)
    press(6);step(30);press(4);step(900);press(4);step(600);press(6);step(300)
elif args.inventory:
    step(90);press(10);step(600);press(3);step(300)
    for i in range(4):press(12)
    press(2);step(120);press(3)
    for i in range(3):press(12)
    step(600);press(0);step(120);press(3);step(600);press(1)
elif args.turnin:
    step(90);press(10);step(450);press(3);step(450);press(1)  # Inspect bags and equipment.
    press(2);step(450);press(0);step(900)  # McBride's quest list, then reward dialog.
    press(0);step(600)  # Claim A Threat Within.
    press(2);step(450);press(0);step(600);press(0);step(600)  # Accept the follow-up.
elif args.quest:
    step(90);press(2);step(900)  # Open the nearby questgiver and allow a screenshot.
    press(0);step(90);press(0);step(600)  # Select if a list is shown, then accept.
elif args.scene:
    step(60);press(8)  # Hide diagnostics for actual scene inspection.
    step(45,axes=(0,0,32767,0,0,0));step(300)
    step(120,axes=(0,-32767,0,0,0,0));step(1800)
elif args.enabled:
    step(60)
    for layer in (1,2,3):
        for button in (0,1,2,3,11,12,13,14):
            step(buttons=1<<button,axes=(0,0,0,0,32767 if layer&1 else 0,32767 if layer&2 else 0));step()
    step(120,axes=(0,-32767,0,0,0,0))
    step(60,axes=(0,0,12000,0,0,0))
    press(0);step(30)
    press(6);step(30)  # Open controls.
    press(11);press(14);press(2)  # Adjust dead zone, remap, save.
    step(300)  # Allow screenshot inspection of the saved settings.
    press(4);step(900)  # Log out, allowing the server's 20-second countdown.
    press(4);step(300)  # Reconnect and recover the persisted position.
    press(6);step(60)
scenario_magic=(b'WXCH' if args.soak else b'WXCG' if args.corpse else b'WXCF' if args.walk else b'WXCE' if args.quest_loop else b'WXCD' if args.avatar_switch else b'WXCC' if args.spellbook else b'WXCB' if args.camp_quest else b'WXCA' if args.starter_quest else b'WXC9' if args.character else b'WXC8' if args.camera else b'WXC7' if args.appearance else b'WXC6' if args.vendor else b'WXC5' if args.death else b'WXC4' if args.kobold_turnin else b'WXC3' if args.boundary else b'WXC2' if args.kobolds else b'WXC1' if args.combat else b'NONE')
if len(records)>1024 or frame>108000:p.error('Replay exceeds the bounded native format')
destination.mkdir(parents=True,exist_ok=True)
for name,payload in (('LOGIN.BIN',login),('SOAK.BIN',soak),('FONT.BIN',font),('TESTCHAR.BIN',test_character),('CAMP.RTE',route),('scenario.bin',scenario_magic)):
    (destination/name).write_bytes(payload)
out=destination/'input.rpl'
out.write_bytes(struct.pack('<4sII',b'WXR1',len(records),frame)+b''.join(records))

print('Account/realm controller replay enabled.' if args.login_replay else f'Input replay: {frame} frames' if records else
      f'State-driven controller scenario: {scenario_magic.decode("ascii")}' if scenario_magic!=b'NONE' else
      'Live controller input selected.')
