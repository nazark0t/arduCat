"""Upload a bounded three-second WAV and play only after checksum confirmation."""
import argparse
import array
import sys
import time
import wave
from pathlib import Path

import serial

parser = argparse.ArgumentParser()
parser.add_argument('--port', required=True)
parser.add_argument('--wav', type=Path, required=True)
parser.add_argument('--peak', type=int, default=12000, choices=range(1, 16001), metavar='1..16000')
args = parser.parse_args()
with wave.open(str(args.wav), 'rb') as wav:
    if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getnframes()) != (1, 2, 16000, 48000):
        raise ValueError('Expected mono PCM16 16kHz WAV of exactly 3 seconds')
    samples = array.array('h', wav.readframes(48000))
if sys.byteorder != 'little':
    samples.byteswap()
mean = sum(samples) / len(samples)
peak = max(abs(value - mean) for value in samples)
if peak == 0:
    raise ValueError('Silent source')
gain = min(100.0, args.peak / peak)
result = array.array('h')
for n, value in enumerate(samples):
    fade = min(1.0, n / 160, (len(samples) - 1 - n) / 160)
    result.append(round((value - mean) * gain * fade))
if sys.byteorder != 'little':
    result.byteswap()
payload = result.tobytes()

def expect(device, prefix):
    deadline = time.monotonic() + 10
    while time.monotonic() < deadline:
        line = device.readline().strip()
        if line.startswith(b'ERROR'):
            raise RuntimeError(line)
        if line.startswith(prefix):
            return line
    raise TimeoutError(f'Expected {prefix!r}')

with serial.Serial(args.port, 921600, timeout=1, write_timeout=2) as device:
    device.dtr = False
    device.rts = False
    time.sleep(2)
    device.reset_input_buffer()
    device.write(b'p')
    expect(device, b'UPLOAD 96000')
    for offset in range(0, len(payload), 1024):
        expect(device, b'NEXT')
        chunk = payload[offset:offset + 1024]
        if device.write(chunk) != len(chunk):
            raise RuntimeError('Incomplete UART write')
    confirmation = expect(device, b'RECEIVED ')
    if int(confirmation.split()[1]) != sum(payload):
        raise RuntimeError('Checksum mismatch; playback not requested')
    print(f'Upload verified: {len(payload)} bytes; gain={gain:.2f}; peak limit={args.peak}', flush=True)
    device.write(b'g')
    print(expect(device, b'PLAYING').decode(), flush=True)
    print(expect(device, b'PLAY DONE').decode(), flush=True)
