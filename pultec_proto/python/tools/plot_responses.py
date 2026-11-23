"""Plots the key responses against the published targets -> docs/*.png"""
import os, sys
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from pultec import Params, eq_response_db, saturation as S

out = os.path.join(os.path.dirname(__file__), "..", "..", "docs")
f = np.geomspace(10, 24000, 2000)
fig, ax = plt.subplots(1, 3, figsize=(16, 4.2))

for fs in (20, 30, 60, 100):
    ax[0].semilogx(f, eq_response_db(Params(lf_freq=fs, lf_boost=2.5, lf_cut=2.5), f), label=f"{fs} Hz")
ax[0].set_title("Low: boost 2.5 + atten 2.5\n(SOS target: +3 dB shelf, -2 dB dip)")
for bw in (0, 5, 10):
    ax[1].semilogx(f, eq_response_db(Params(hf_freq=8000, hf_boost=10, hf_bw=bw), f), label=f"bandwidth {bw}")
ax[1].set_title("HF boost @ 8 kHz, max\n(SOS target: 9 dB narrow vs broad)")
for fc in (5000, 10000, 20000):
    ax[2].semilogx(f, eq_response_db(Params(hf_cut_freq=fc, hf_cut=10), f), label=f"{fc // 1000} kHz")
ax[2].set_title("HF atten, max\n(Manley target: -12 dB at selected f)")
for a in ax:
    a.grid(True, which="both", alpha=.3); a.legend(); a.set_xlabel("Hz"); a.set_ylabel("dB")
plt.tight_layout(); plt.savefig(os.path.join(out, "eq_curves.png"), dpi=110)

fig, ax = plt.subplots(1, 2, figsize=(11, 4.2))
lv = np.arange(-40, 1, 4.0); n, m = 1 << 15, 171
t = np.arange(n) / n
for sat in (1, 3, 6):
    thd = [S.thd_percent(S.tube(10 ** (d / 20) * np.sin(2 * np.pi * m * t), S.k_calibrated() * sat), m, n) for d in lv]
    ax[0].semilogy(lv, thd, marker="o", ms=3, label=f"saturation {sat}")
ax[0].axvline(-12, color="gray", ls=":"); ax[0].set_title("Tube THD vs level (1 kHz)\n0.10 % at -12 dBFS (+10 dBu) when saturation = 1")
ax[0].set_xlabel("input dBFS"); ax[0].set_ylabel("THD %"); ax[0].grid(True, which="both", alpha=.3); ax[0].legend()
p = Params(hf_freq=16000, hf_boost=10, hf_bw=0); ff = np.geomspace(100, 20000, 600)
ref = eq_response_db(p, ff)
for fs in (48000, 96000, 192000, 384000):
    ax[1].semilogx(ff, eq_response_db(p, ff, fs=fs) - ref, label=f"{fs // 1000} kHz")
ax[1].set_title("Digital vs analog error, HF boost 16 kHz (sharp, max)\nprewarped bilinear"); ax[1].set_xlabel("Hz"); ax[1].set_ylabel("dB error")
ax[1].grid(True, which="both", alpha=.3); ax[1].legend()
plt.tight_layout(); plt.savefig(os.path.join(out, "saturation_and_oversampling.png"), dpi=110)
print("plots written to docs/")
