"""Analog prototypes (s-domain) of the four Pultec filters + digitisation.

Behavioural model: each filter is an analog section whose shape follows the
published measurements (see constants.py). The real circuit interaction (shared
passive network loading) is NOT modelled here; netlist.py is the hook for that.
"""
import numpy as np
from scipy.optimize import brentq
from scipy.signal import bilinear

from . import constants as C

# Z0 = sqrt(L/C) per inductor tap, regressed on log f (extrapolates to 3 kHz)
_taps = sorted(C.HF_BOOST_LC)
_z0 = np.array([np.sqrt(C.HF_BOOST_LC[f][0] / C.HF_BOOST_LC[f][1]) for f in _taps])
_Z_FIT = np.polyfit(np.log(_taps), _z0, 1)
_Z_MEAN = float(_z0.mean())

Q_SHARP, Q_BROAD = 2.5, 0.6   # [assumed] at mean tap impedance


def taper(pos, exponent=1.0):
    return (np.clip(pos, 0.0, 10.0) / 10.0) ** exponent


def low_shelf2(f0, gain_db, q):
    """Resonant 2nd-order low shelf (RBJ analog prototype); DC gain = gain_db."""
    A = 10 ** (gain_db / 40)
    w0 = 2 * np.pi * f0
    b = A * np.array([1 / w0**2, np.sqrt(A) / (q * w0), A])
    a = np.array([A / w0**2, np.sqrt(A) / (q * w0), 1.0])
    return b, a


def low_cut1(fc, depth_db):
    """1st-order low shelf CUT: DC gain -depth_db, 0 dB at HF, centre fc."""
    r = 10 ** (-depth_db / 20)
    wc = 2 * np.pi * fc
    wz, wp = wc * np.sqrt(r), wc / np.sqrt(r)
    return r * np.array([1 / wz, 1.0]), np.array([1 / wp, 1.0])


def peaking(f0, gain_db, q):
    """Analog peaking bell (RBJ prototype)."""
    A = 10 ** (gain_db / 40)
    w0 = 2 * np.pi * f0
    return (np.array([1.0, A * w0 / q, w0**2]),
            np.array([1.0, w0 / (A * q), w0**2]))


def high_cut1(f_sel, depth_db, frac=C.HF_CUT_AT_SEL_FRAC):
    """1st-order high shelf CUT with `frac*depth` dB reached at f_sel."""
    r = 10 ** (-depth_db / 20)
    target = 10 ** (-frac * depth_db / 20)
    w = 2 * np.pi * f_sel

    def g(u):                      # u = wz / w
        return np.sqrt((1 + 1 / u**2) / (1 + 1 / (r * u) ** 2)) - target

    u = brentq(g, 1e-4, 1e4)
    wz = u * w
    wp = r * wz
    return np.array([1 / wz, 1.0]), np.array([1 / wp, 1.0])


def hf_q(f0, bw_pos):
    """Q from bandwidth knob (0 sharp..10 broad) scaled by tap impedance."""
    q_base = np.exp(np.interp(bw_pos, [0, 10], [np.log(Q_SHARP), np.log(Q_BROAD)]))
    z = np.polyval(_Z_FIT, np.log(f0))
    return q_base * z / _Z_MEAN


def analog_sections(p):
    secs = []
    # Low boost
    gb = C.LF_BOOST_MAX_DB * taper(p.lf_boost, p.lf_boost_taper)
    if gb > 1e-3:
        secs.append(low_shelf2(p.lf_freq, gb, p.lf_boost_q) + (p.lf_freq,))
    # Low cut (two staggered 1st-order shelves = wide, shallow slope)
    gc = C.LF_CUT_MAX_DB * taper(p.lf_cut, p.lf_cut_taper)
    if gc > 1e-3:
        secs.append(low_cut1(p.lf_cut_k1 * p.lf_freq, p.lf_cut_split * gc) + (p.lf_cut_k1 * p.lf_freq,))
        secs.append(low_cut1(p.lf_cut_k2 * p.lf_freq, (1 - p.lf_cut_split) * gc) + (p.lf_cut_k2 * p.lf_freq,))
    # High boost: peak falls as bandwidth widens (SOS: 9 dB spread at max)
    gh = C.HF_BOOST_MAX_DB * taper(p.hf_boost)
    if gh > 1e-3:
        gpeak = gh * (1 - (C.HF_BW_SPREAD_DB / C.HF_BOOST_MAX_DB) * p.hf_bw / 10)
        secs.append(peaking(p.hf_freq, gpeak, hf_q(p.hf_freq, p.hf_bw)) + (p.hf_freq,))
    # High cut
    gx = C.HF_CUT_MAX_DB * taper(p.hf_cut)
    if gx > 1e-3:
        secs.append(high_cut1(p.hf_cut_freq, gx) + (p.hf_cut_freq,))
    return secs


def analog_response_db(secs, f):
    s = 2j * np.pi * np.asarray(f, float)
    H = np.ones_like(s)
    for b, a, _fp in secs:
        H = H * np.polyval(b, s) / np.polyval(a, s)
    return 20 * np.log10(np.abs(H))


def digital_sos(secs, fs, prewarp=True):
    """Bilinear transform per section, prewarped so each section's key
    frequency (peak / corner) lands exactly where the analog one does."""
    rows = []
    for b, a, fp in secs:
        fs_eff = fs
        if prewarp and fp < 0.45 * fs:
            fs_eff = np.pi * fp / np.tan(np.pi * fp / fs)
        bz, az = bilinear(b, a, fs_eff)
        bz = np.pad(bz, (0, 3 - len(bz)))
        az = np.pad(az, (0, 3 - len(az)))
        rows.append([bz[0], bz[1], bz[2], 1.0, az[1] / az[0], az[2] / az[0]])
        rows[-1][:3] = [v / az[0] for v in rows[-1][:3]]
    return np.array(rows) if rows else np.zeros((0, 6))
