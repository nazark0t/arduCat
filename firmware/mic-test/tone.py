"""Play the firmware's quiet two-tone signal once."""
import argparse
import time

import serial

parser = argparse.ArgumentParser()
parser.add_argument('--port', required=True)
args = parser.parse_args()
with serial.Serial(args.port, 921600, timeout=0.5) as device:
    device.dtr = False
    device.rts = False
    time.sleep(2)
    device.reset_input_buffer()
    device.write(b't')
    deadline = time.monotonic() + 5
    data = bytearray()
    while time.monotonic() < deadline:
        data.extend(device.read(device.in_waiting or 1))
        if b'TONE DONE\n' in data:
            break
    print(data.decode('ascii', errors='backslashreplace'))
    if b'TONE DONE\n' not in data:
        raise TimeoutError('Firmware did not confirm tone completion')
