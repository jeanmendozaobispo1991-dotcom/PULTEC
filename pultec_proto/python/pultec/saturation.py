"""Valve stage + output transformer (behavioural, level dependent).

tube():   asymmetric tanh, normalised to unity small-signal gain. Distortion
          (mostly H2) grows with level and flattens at high levels (no threshold
          explosion), matching SOS 2019 (H2 rises with level, others ~constant).
xfmr():   low-frequency flux-style saturation: only the content below ~120 Hz is
          soft-clipped, giving level-dependent bass harmonics (odd).
Both operate at the oversampled rate.
"""
import numpy as np
from scipy.optimize import brentq
from scipy.signal import lfilter

from . import constants as C

TUBE_BIAS = 0.3          # [assumed] asymmetry
XFMR_CORNER_HZ = 120.0   # [assumed]
XFMR_THRESHOLD = 1.0     # [assumed] linear full-scale where flux saturation starts


def tube(x, k, b=TUBE_BIAS):
    if k <= 0:
        return x
    t = np.tanh(b)
    return (np.tanh(k * x + b) - t) / ((1 - t * t) * k)


def xfmr(x, fs, strength, axis=0):
    if strength <= 0:
        return x
    a = np.exp(-2 * np.pi * XFMR_CORNER_HZ / fs)
    lp = lfilter([1 - a], [1, -a], x, axis=axis)
    thr = XFMR_THRESHOLD / np.sqrt(strength)
    return (x - lp) + thr * np.tanh(lp / thr)


def thd_percent(y, m, n):
    """THD of a coherent sine (m cycles in n samples), harmonics 2..10."""
    Y = np.abs(np.fft.rfft(y)) / n * 2
    h1 = Y[m]
    hs = [Y[m * k] for k in range(2, 11) if m * k < len(Y)]
    return 100 * np.sqrt(np.sum(np.square(hs))) / h1


def calibrate_k(fs=192000, n=1 << 15, m=171, level_dbfs=-12.0, target_pct=0.10):
    """Drive K so a ~1 kHz sine at +10 dBu (-12 dBFS) gives `target_pct` THD."""
    t = np.arange(n) / n
    x = 10 ** (level_dbfs / 20) * np.sin(2 * np.pi * m * t)
    f = lambda k: thd_percent(tube(x, k), m, n) - target_pct
    return brentq(f, 1e-3, 5.0)


_K_CAL = None


def k_calibrated():
    global _K_CAL
    if _K_CAL is None:
        _K_CAL = calibrate_k(target_pct=C.THD_SPEC_PERCENT * 2 / 3)  # 0.10 %
    return _K_CAL
