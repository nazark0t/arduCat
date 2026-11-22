"""Request one three-second recording and save PCM plus a normalized preview."""
import argparse
import array
import math
import sys
import time
import wave
from pathlib import Path

import serial

parser = argparse.ArgumentParser()
parser.add_argument('--port', required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
with serial.Serial(args.port, 921600, timeout=1) as device:
    device.dtr = False
    device.rts = False
    time.sleep(2)
    device.reset_input_buffer()
    device.write(b'r')
    deadline = time.monotonic() + 15
    while time.monotonic() < deadline:
        line = device.readline()
        print(line.decode('ascii', errors='replace').strip(), flush=True)
        if line == b'AUDIO 96000\n':
            break
        if line.startswith(b'ERROR'):
            raise RuntimeError(line)
    else:
        raise TimeoutError('No AUDIO header from firmware')
    data = bytearray()
    while len(data) < 96000 and time.monotonic() < deadline:
        data.extend(device.read(96000 - len(data)))
    if len(data) != 96000:
        raise RuntimeError(f'Incomplete recording: {len(data)}/96000 bytes')
    if device.read(6) != b'\nDONE\n':
        raise RuntimeError('Invalid recording trailer')

samples = array.array('h', data)
if sys.byteorder != 'little':
    samples.byteswap()
mean = sum(samples) / len(samples)
rms = math.sqrt(sum((value - mean) ** 2 for value in samples) / len(samples))
peak = max(abs(value) for value in samples)
print(f'samples={len(samples)} mean={mean:.2f} AC_RMS={rms:.2f} peak={peak} '
      f'nonzero={sum(value != 0 for value in samples)} '
      f'clipped={sum(abs(value) >= 32767 for value in samples)}')
args.output.parent.mkdir(parents=True, exist_ok=True)
with wave.open(str(args.output), 'wb') as wav:
    wav.setparams((1, 2, 16000, 0, 'NONE', 'not compressed'))
    wav.writeframes(data)
centered_peak = max(abs(value - mean) for value in samples)
if centered_peak:
    gain = min(100.0, 26000 / centered_peak)
    normalized = array.array('h', (max(-32768, min(32767, round((value - mean) * gain))) for value in samples))
    if sys.byteorder != 'little':
        normalized.byteswap()
    preview = args.output.with_stem(args.output.stem + '-preview')
    with wave.open(str(preview), 'wb') as wav:
        wav.setparams((1, 2, 16000, 0, 'NONE', 'not compressed'))
        wav.writeframes(normalized.tobytes())
    print(f'Preview gain={gain:.2f}; {preview}')
print(args.output)
