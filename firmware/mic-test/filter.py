"""Create a separate PCM16 speech test with a second-order Butterworth high-pass."""
import argparse
import array
import math
import sys
import wave
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('source', type=Path)
parser.add_argument('output', type=Path)
parser.add_argument('--cutoff', type=float, default=150)
args = parser.parse_args()
if args.source.resolve() == args.output.resolve():
    raise ValueError('Output must differ from source')
with wave.open(str(args.source), 'rb') as wav:
    if wav.getnchannels() != 1 or wav.getsampwidth() != 2:
        raise ValueError('Expected mono PCM16')
    rate = wav.getframerate()
    samples = array.array('h', wav.readframes(wav.getnframes()))
if not 0 < args.cutoff < rate / 2:
    raise ValueError('Cutoff outside valid range')
if sys.byteorder != 'little':
    samples.byteswap()
omega = 2 * math.pi * args.cutoff / rate
cosine = math.cos(omega)
alpha = math.sin(omega) / math.sqrt(2)
b0 = (1 + cosine) / (2 * (1 + alpha))
b1, b2 = -2 * b0, b0
a1, a2 = -2 * cosine / (1 + alpha), (1 - alpha) / (1 + alpha)
mean = sum(samples) / len(samples)
x1 = x2 = y1 = y2 = 0.0
result = array.array('h')
clipped = 0
for sample in samples:
    x = sample - mean
    y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
    x2, x1, y2, y1 = x1, x, y1, y
    value = round(y)
    clipped += not -32768 <= value <= 32767
    result.append(max(-32768, min(32767, value)))
if sys.byteorder != 'little':
    result.byteswap()
args.output.parent.mkdir(parents=True, exist_ok=True)
with wave.open(str(args.output), 'wb') as wav:
    wav.setparams((1, 2, rate, 0, 'NONE', 'not compressed'))
    wav.writeframes(result.tobytes())
print(f'High-pass {args.cutoff:g}Hz; clipped={clipped}; {args.output}')
