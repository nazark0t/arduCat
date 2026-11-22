"""Inspect PCM16 mono WAVs with a Hann-windowed radix-2 FFT (stdlib only)."""
import argparse
import array
import cmath
import json
import math
import sys
import wave
from pathlib import Path

def analyze(path):
    with wave.open(str(path), 'rb') as wav:
        if wav.getnchannels() != 1 or wav.getsampwidth() != 2:
            raise ValueError('Expected mono PCM16')
        rate = wav.getframerate()
        values = array.array('h', wav.readframes(wav.getnframes()))
    if sys.byteorder != 'little':
        values.byteswap()
    mean = sum(values) / len(values)
    centered = [v - mean for v in values]
    rms = math.sqrt(sum(v * v for v in centered) / len(centered))
    size = 1 << (len(values) - 1).bit_length()
    spectrum = [complex(v * (0.5 - 0.5 * math.cos(2 * math.pi * i / (len(values) - 1))))
                for i, v in enumerate(centered)]
    spectrum.extend([0j] * (size - len(spectrum)))
    j = 0
    for i in range(1, size):
        bit = size >> 1
        while j & bit:
            j ^= bit
            bit >>= 1
        j ^= bit
        if i < j:
            spectrum[i], spectrum[j] = spectrum[j], spectrum[i]
    length = 2
    while length <= size:
        step = cmath.exp(-2j * math.pi / length)
        half = length // 2
        for start in range(0, size, length):
            weight = 1 + 0j
            for k in range(half):
                a = spectrum[start + k]
                b = weight * spectrum[start + k + half]
                spectrum[start + k], spectrum[start + k + half] = a + b, a - b
                weight *= step
        length *= 2
    power = [abs(v) ** 2 for v in spectrum[:size // 2 + 1]]
    total = sum(power[1:-1])
    bands = {}
    for lower, upper in [(0, 100), (100, 200), (200, 400), (400, 4000), (4000, rate / 2 + 1)]:
        energy = sum(p for k, p in enumerate(power) if k and lower <= k * rate / size < upper)
        bands[f'{lower}-{upper:g}Hz_percent'] = round(100 * energy / total, 2) if total else 0
    return {'file': str(path), 'seconds': len(values) / rate, 'sample_rate': rate,
            'mean': round(mean, 2), 'rms': round(rms, 2), 'peak': max(abs(v) for v in values),
            'clipped': sum(v <= -32768 or v >= 32767 for v in values),
            'rms_dbfs': round(20 * math.log10(rms / 32768), 2) if rms else None,
            'windowed_spectral_energy': bands}

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('files', nargs='+', type=Path)
    args = parser.parse_args()
    print(json.dumps([analyze(p) for p in args.files], indent=2))
