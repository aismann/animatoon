#!/usr/bin/env python3
"""Erzeugt die Soundeffekte (16-bit mono WAV, 44.1 kHz) synthetisch. Aufruf: python3 make_sfx.py ../Content/sfx"""
import sys, wave
import numpy as np

SR = 44100
out = sys.argv[1] if len(sys.argv) > 1 else "."
rng = np.random.default_rng(7)

def t(d): return np.linspace(0, d, int(SR * d), endpoint=False)
def env(n, a=0.005, d=1.0):
    x = np.arange(n) / SR
    e = np.minimum(1, x / a) * np.exp(-x * (5.0 / d))
    return e
def tone(f0, f1, d, vol=0.6, a=0.004, decay=None):
    x = t(d); f = np.linspace(f0, f1, len(x)); ph = 2 * np.pi * np.cumsum(f) / SR
    return vol * np.sin(ph) * env(len(x), a, decay or d)
def noise(d, vol=0.4, lp=0.2):
    n = rng.standard_normal(int(SR * d)); y = np.zeros_like(n); s = 0.0
    for i, v in enumerate(n): s += lp * (v - s); y[i] = s
    y /= max(1e-6, np.abs(y).max()); return vol * y * env(len(y), 0.002, d)
def seq(parts, gap=0.0):
    return np.concatenate([np.concatenate([p, np.zeros(int(SR * gap))]) for p in parts])
def save(name, y):
    y = np.clip(y / max(1.0, np.abs(y).max()) * 0.9, -1, 1)
    with wave.open(f"{out}/{name}.wav", "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes((y * 32767).astype("<i2").tobytes())

save("click",  tone(1100, 900, 0.05, 0.5))
save("select", seq([tone(600, 620, 0.06, 0.5), tone(900, 920, 0.08, 0.5)]))
save("start",  tone(280, 900, 0.22, 0.55, 0.01, 0.3))
save("hit_soft", np.pad(noise(0.05, 0.25, 0.4), (0, int(SR * 0.09) - int(SR * 0.05))) + tone(220, 120, 0.09, 0.6))
save("hit_hard", np.pad(noise(0.08, 0.5, 0.5), (0, int(SR * 0.16) - int(SR * 0.08))) + tone(150, 60, 0.16, 0.8, 0.002, 0.12))
sp = tone(520, 110, 0.18, 0.55, 0.006, 0.12) + np.pad(noise(0.12, 0.3, 0.12), (0, int(SR * 0.18) - int(SR * 0.12)))
save("splat", sp)
save("win",    seq([tone(f, f, 0.14, 0.5, 0.005, 0.2) for f in (523.25, 659.25, 783.99, 1046.5)], 0.02))
save("level",  seq([tone(659.25, 659.25, 0.09, 0.5), tone(880, 880, 0.14, 0.5, 0.005, 0.2)], 0.01))
save("pop",    tone(800, 500, 0.06, 0.55))
save("erase",  tone(380, 160, 0.09, 0.55))
save("goal",   seq([tone(700, 700, 0.06, 0.5), tone(1050, 1050, 0.1, 0.5, 0.005, 0.15)], 0.0))
