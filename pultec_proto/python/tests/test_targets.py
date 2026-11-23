"""Checks the prototype against the targets gathered from the references.
Run:  python -m pytest tests -q     (or: python tests/test_targets.py)
"""
import os, sys
import numpy as np
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from pultec import Params, eq_response_db, process
from pultec import constants as C, saturation as S
from pultec.netlist import hf_tap_resonance_hz, ac_response

F = np.geomspace(10, 24000, 4000)


def test_hf_taps_match_nominal_frequencies():
    for nominal, f0 in hf_tap_resonance_hz().items():
        assert abs(f0 / nominal - 1) < 0.01, (nominal, f0)


def test_mna_series_resonator_peaks_at_f0():
    L, Cc = C.HF_BOOST_LC[10000]
    els = [('L', 'in', 'x', L), ('C', 'x', 'out', Cc), ('R', 'out', 'gnd', 600.0)]
    f = np.geomspace(2000, 40000, 3000)
    h = np.abs(ac_response(els, 'in', 'out', 600.0, f))
    assert abs(f[np.argmax(h)] / 10000 - 1) < 0.02


def test_lf_boost_and_cut_not_mirror_images():
    for fs in C.LF_FREQS_HZ:
        pb = eq_response_db(Params(lf_freq=fs, lf_boost=10), F)
        pc = eq_response_db(Params(lf_freq=fs, lf_cut=10), F)
        # cut corner above boost corner (SOS: ~half octave) -> cut affects higher f
        fb = F[np.argmin(np.abs(pb - pb[0] / 2))]
        fc = F[np.argmin(np.abs(pc - pc[0] / 2))]
        assert fc > fb * 1.2, (fs, fb, fc)


def test_pultec_trick_gives_boost_with_midrange_dip():
    for fs in C.LF_FREQS_HZ:
        r = eq_response_db(Params(lf_freq=fs, lf_boost=2.5, lf_cut=2.5), F)
        assert 2.0 < r[0] < 4.0, (fs, r[0])               # ~ +3 dB shelf (SOS)
        i = np.argmin(r)
        assert -2.6 < r[i] < -1.4, (fs, r[i])              # ~ -2 dB dip (SOS)
        assert 2.5 * fs < F[i] < 10 * fs, (fs, F[i])       # dip a few x selector


def test_hf_boost_bandwidth_spread_is_9db():
    f0 = 5000
    sharp = eq_response_db(Params(hf_freq=f0, hf_boost=10, hf_bw=0), F).max()
    broad = eq_response_db(Params(hf_freq=f0, hf_boost=10, hf_bw=10), F).max()
    assert abs(sharp - 20) < 0.3 and abs((sharp - broad) - 9) < 0.3, (sharp, broad)


def test_hf_bandwidth_narrows_at_higher_frequency():
    def octaves(f0):
        r = eq_response_db(Params(hf_freq=f0, hf_boost=10, hf_bw=5), F)
        above = F[r > r.max() - 3]
        return np.log2(above.max() / above.min())
    assert octaves(16000) < octaves(4000)


def test_hf_cut_hits_minus12_at_selected_frequency():
    for f0 in C.HF_CUT_FREQS_HZ:
        r = eq_response_db(Params(hf_cut=10, hf_cut_freq=f0), np.array([f0, 1e7]))
        assert abs(r[0] + 12) < 0.3 and abs(r[1] + 21) < 0.3, (f0, r)


def test_oversampling_reduces_bilinear_cramping():
    p = Params(hf_freq=16000, hf_boost=10, hf_bw=0)
    f = np.geomspace(100, 20000, 800)
    ref = eq_response_db(p, f)
    errs = [np.max(np.abs(eq_response_db(p, f, fs=fs) - ref)) for fs in (48000, 96000, 192000)]
    assert errs[0] > errs[1] > errs[2] and errs[2] < 0.6, errs


def _sine(db, hz, fs, n=1 << 15):
    m = int(round(hz * n / fs))
    t = np.arange(n) / n
    return 10 ** (db / 20) * np.sin(2 * np.pi * m * t), m, n


def test_tube_thd_calibrated_and_monotonic_without_explosion():
    fs = 192000
    t0 = None
    prev = 0
    for db in (-40, -30, -20, -12, -6, 0):
        x, m, n = _sine(db, 1000, fs)
        thd = S.thd_percent(S.tube(x, S.k_calibrated()), m, n)
        assert thd > prev, db
        prev = thd
        if db == -12:
            t0 = thd
    assert abs(t0 - 0.10) < 0.01, t0
    x, m, n = _sine(0, 1000, fs)
    assert prev < 1.0, prev        # no threshold explosion at 0 dBFS


def test_tube_h2_dominates_h3():
    x, m, n = _sine(-6, 1000, 192000)
    Y = np.abs(np.fft.rfft(S.tube(x, S.k_calibrated())))
    assert Y[2 * m] > 3 * Y[3 * m]


def test_transformer_distorts_bass_more_than_mids():
    fs = 192000
    out = {}
    for hz in (100, 1000):
        x, m, n = _sine(-6, hz, fs)
        out[hz] = S.thd_percent(S.xfmr(x, fs, 1.0), m, n)
    assert out[100] > 5 * out[1000], out


def test_neutral_settings_are_near_transparent_and_deterministic():
    fs = 48000
    t = np.arange(fs) / fs
    x = 0.1 * np.sin(2 * np.pi * 440 * t) + 0.05 * np.sin(2 * np.pi * 3000 * t)
    y = process(x, fs, Params())
    err = np.sqrt(np.mean((y[2000:-2000] - x[2000:-2000]) ** 2)) / np.sqrt(np.mean(x ** 2))
    assert err < 0.01, err
    assert np.allclose(y, process(x, fs, Params()))


if __name__ == "__main__":
    fails = 0
    for name, fn in sorted(globals().items()):
        if name.startswith("test_"):
            try:
                fn(); print("PASS", name)
            except AssertionError as e:
                fails += 1; print("FAIL", name, e)
    sys.exit(1 if fails else 0)
