import numpy as np
from scipy.signal import resample_poly, sosfilt, sosfreqz

from . import analog, saturation


def eq_response_db(p, freqs, fs=None):
    """EQ network response. fs=None -> analog; fs given -> digital at that rate."""
    secs = analog.analog_sections(p)
    if fs is None:
        return analog.analog_response_db(secs, freqs)
    sos = analog.digital_sos(secs, fs)
    if len(sos) == 0:
        return np.zeros(len(freqs))
    _, h = sosfreqz(sos, worN=np.asarray(freqs, float), fs=fs)
    return 20 * np.log10(np.abs(h))


def loudness_comp_db(p, n=400):
    """Gain that offsets the EQ's average loudness change (A-weighted, pink)."""
    f = np.geomspace(30, 16000, n)
    f2 = f**2
    ra = (12194**2 * f2**2) / ((f2 + 20.6**2) * np.sqrt((f2 + 107.7**2) * (f2 + 737.9**2)) * (f2 + 12194**2))
    w = ra**2 / f                      # A-weight power x pink density
    g = 10 ** (eq_response_db(p, f) / 10)
    return -10 * np.log10(np.sum(w * g) / np.sum(w))


def process(x, fs, p, auto_gain=False):
    """x: (n,) or (n, ch) float array in [-1, 1]. Returns same shape."""
    x = np.asarray(x, float)
    y = x * 10 ** (p.input_gain_db / 20)
    os_ = max(1, int(p.oversampling))
    if os_ > 1:
        y = resample_poly(y, os_, 1, axis=0)
    fs_os = fs * os_
    if p.eq_in:
        sos = analog.digital_sos(analog.analog_sections(p), fs_os)
        if len(sos):
            y = sosfilt(sos, y, axis=0)
    if p.saturation > 0:
        y = saturation.tube(y, saturation.k_calibrated() * p.saturation)
        y = saturation.xfmr(y, fs_os, p.saturation)
    if os_ > 1:
        y = resample_poly(y, 1, os_, axis=0)
    if auto_gain and p.eq_in:
        y = y * 10 ** (loudness_comp_db(p) / 20)
    return y[: len(x)]
