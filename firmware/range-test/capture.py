import argparse, time, sys
sys.stdout.reconfigure(encoding="utf-8", errors="replace")
from pathlib import Path
import serial
p=argparse.ArgumentParser(); p.add_argument('--port',default='COM5');p.add_argument('--output',default='.tools/range-test.log');p.add_argument('--seconds',type=float,default=10);p.add_argument('--no-reset',action='store_true');a=p.parse_args()
s=serial.Serial();s.port=a.port;s.baudrate=115200;s.timeout=0.3;s.dtr=False;s.rts=False
with s:
 if not a.no_reset:
  s.rts=True;time.sleep(.15);s.rts=False
 end=time.monotonic()+a.seconds; data=bytearray()
 while time.monotonic()<end:
  chunk=s.read(s.in_waiting or 1);data.extend(chunk)
  if b'DONE stop=' in data or b'FAIL ' in data:break
 Path(a.output).write_bytes(data)
 print(data.decode('utf-8',errors='replace'))
