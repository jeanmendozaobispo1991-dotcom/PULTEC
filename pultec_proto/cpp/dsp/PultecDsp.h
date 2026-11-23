// Pultec EQP-1A style DSP core. Header-only, no dependencies (C++17).
// Mirrors python/pultec (analog.py, saturation.py) -- keep both in sync;
// cpp/tests/dsp_test.cpp checks the coefficients against python output.
#pragma once
#include <array>
#include <cmath>

namespace pultec {

constexpr double kPi = 3.14159265358979323846;

// ---- spec constants (source tags in python/pultec/constants.py) -------------
constexpr double kLfBoostMaxDb = 16.0;
constexpr double kLfCutMaxDb = 20.0;
constexpr double kHfBoostMaxDb = 20.0;
constexpr double kHfBwSpreadDb = 9.0;
constexpr double kHfCutMaxDb = 21.0;
constexpr double kHfCutAtSelFrac = 12.0 / 21.0;
constexpr double kQSharp = 2.5, kQBroad = 0.6;
constexpr double kTubeBias = 0.3;
constexpr double kXfmrCornerHz = 120.0;
constexpr double kXfmrThreshold = 1.0;
// Drive that gives 0.10 % THD (1 kHz, -12 dBFS, 192 kHz) -- python: k_calibrated()
constexpr double kTubeKCal = 0.027332347982295706;

struct Params {
    double lfFreq = 60, lfBoost = 0, lfCut = 0;
    double hfFreq = 10000, hfBoost = 0, hfBw = 5;
    double hfCutFreq = 10000, hfCut = 0;
    // provisional LF behavioural fit (docs/LF_MODEL.md)
    double lfBoostQ = 0.563, lfBoostTaper = 0.346, lfCutTaper = 0.757;
    double lfCutSplit = 0.638, lfCutK1 = 0.673, lfCutK2 = 11.76;
};

// Analog section, ascending powers of s: N(s)=n0+n1 s+n2 s^2, D(s)=d0+d1 s+d2 s^2
struct Section {
    double n[3] = {1, 0, 0};
    double d[3] = {1, 0, 0};
    double fp = 0;   // key frequency for prewarping (Hz)
    bool active = false;
};

struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
};

inline double taper(double pos, double e) {
    double x = pos < 0 ? 0 : (pos > 10 ? 10 : pos);
    return std::pow(x / 10.0, e);
}

inline Section lowShelf2(double f0, double gDb, double q) {
    double A = std::pow(10.0, gDb / 40), w0 = 2 * kPi * f0, sA = std::sqrt(A);
    Section s;
    s.n[0] = A * A;  s.n[1] = A * sA / (q * w0);  s.n[2] = A / (w0 * w0);
    s.d[0] = 1;      s.d[1] = sA / (q * w0);      s.d[2] = A / (w0 * w0);
    s.fp = f0; s.active = true;
    return s;
}

inline Section lowCut1(double fc, double depthDb) {
    double r = std::pow(10.0, -depthDb / 20), wc = 2 * kPi * fc;
    double wz = wc * std::sqrt(r), wp = wc / std::sqrt(r);
    Section s;
    s.n[0] = r; s.n[1] = r / wz;
    s.d[0] = 1; s.d[1] = 1 / wp;
    s.fp = fc; s.active = true;
    return s;
}

inline Section peaking(double f0, double gDb, double q) {
    double A = std::pow(10.0, gDb / 40), w0 = 2 * kPi * f0;
    Section s;
    s.n[0] = w0 * w0; s.n[1] = A * w0 / q; s.n[2] = 1;
    s.d[0] = w0 * w0; s.d[1] = w0 / (A * q); s.d[2] = 1;
    s.fp = f0; s.active = true;
    return s;
}

inline Section highCut1(double fSel, double depthDb, double frac = kHfCutAtSelFrac) {
    double r = std::pow(10.0, -depthDb / 20);
    double target = std::pow(10.0, -frac * depthDb / 20);
    double w = 2 * kPi * fSel;
    auto g = [&](double u) {
        return std::sqrt((1 + 1 / (u * u)) / (1 + 1 / (r * u * r * u))) - target;
    };
    double lo = std::log(1e-4), hi = std::log(1e4);   // g(lo)<0<g(hi), monotonic
    for (int i = 0; i < 200; ++i) {
        double mid = 0.5 * (lo + hi);
        (g(std::exp(mid)) < 0 ? lo : hi) = mid;
    }
    double wz = std::exp(0.5 * (lo + hi)) * w, wp = r * wz;
    Section s;
    s.n[0] = 1; s.n[1] = 1 / wz;
    s.d[0] = 1; s.d[1] = 1 / wp;
    s.fp = fSel; s.active = true;
    return s;
}

// Q of the HF boost: knob (0 sharp..10 broad) scaled by tap impedance sqrt(L/C),
// regressed on log f from the published L/C taps (python: analog.hf_q).
inline double hfQ(double f0, double bwPos) {
    static const double taps[6] = {4000, 5000, 8000, 10000, 12000, 16000};
    static const double L[6] = {87.9e-3, 67.5e-3, 48.2e-3, 31.4e-3, 31.4e-3, 29.9e-3};
    static const double C[6] = {18e-9, 15e-9, 8.2e-9, 8.2e-9, 5.6e-9, 3.3e-9};
    double sx = 0, sy = 0, sxx = 0, sxy = 0, zm = 0;
    for (int i = 0; i < 6; ++i) {
        double x = std::log(taps[i]), y = std::sqrt(L[i] / C[i]);
        sx += x; sy += y; sxx += x * x; sxy += x * y; zm += y;
    }
    double n = 6, slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    double icpt = (sy - slope * sx) / n;
    zm /= n;
    double t = (bwPos < 0 ? 0 : (bwPos > 10 ? 10 : bwPos)) / 10.0;
    double qBase = std::exp(std::log(kQSharp) + t * (std::log(kQBroad) - std::log(kQSharp)));
    return qBase * (slope * std::log(f0) + icpt) / zm;
}

// Fixed roles so filter state survives knob moves: 0 LF boost, 1-2 LF cut, 3 HF boost, 4 HF cut
using SectionSet = std::array<Section, 5>;

inline SectionSet design(const Params& p) {
    SectionSet s{};
    double gb = kLfBoostMaxDb * taper(p.lfBoost, p.lfBoostTaper);
    if (gb > 1e-3) s[0] = lowShelf2(p.lfFreq, gb, p.lfBoostQ);
    double gc = kLfCutMaxDb * taper(p.lfCut, p.lfCutTaper);
    if (gc > 1e-3) {
        s[1] = lowCut1(p.lfCutK1 * p.lfFreq, p.lfCutSplit * gc);
        s[2] = lowCut1(p.lfCutK2 * p.lfFreq, (1 - p.lfCutSplit) * gc);
    }
    double gh = kHfBoostMaxDb * taper(p.hfBoost, 1.0);
    if (gh > 1e-3) {
        double gpeak = gh * (1 - (kHfBwSpreadDb / kHfBoostMaxDb) * p.hfBw / 10);
        s[3] = peaking(p.hfFreq, gpeak, hfQ(p.hfFreq, p.hfBw));
    }
    double gx = kHfCutMaxDb * taper(p.hfCut, 1.0);
    if (gx > 1e-3) s[4] = highCut1(p.hfCutFreq, gx);
    return s;
}

// Bilinear transform with prewarp at the section's key frequency.
inline Biquad toDigital(const Section& s, double fs) {
    if (!s.active) return {};
    double fsEff = fs;
    if (s.fp < 0.45 * fs) fsEff = kPi * s.fp / std::tan(kPi * s.fp / fs);
    double c = 2 * fsEff, c2 = c * c;
    const double* n = s.n; const double* d = s.d;
    if (n[2] == 0 && d[2] == 0) {   // 1st order: no (1+z^-1) pole/zero pair at Nyquist
        double da0 = d[0] + d[1] * c;
        return {(n[0] + n[1] * c) / da0, (n[0] - n[1] * c) / da0, 0.0, (d[0] - d[1] * c) / da0, 0.0};
    }
    double nb0 = n[0] + n[1] * c + n[2] * c2, nb1 = 2 * n[0] - 2 * n[2] * c2,
           nb2 = n[0] - n[1] * c + n[2] * c2;
    double da0 = d[0] + d[1] * c + d[2] * c2, da1 = 2 * d[0] - 2 * d[2] * c2,
           da2 = d[0] - d[1] * c + d[2] * c2;
    return {nb0 / da0, nb1 / da0, nb2 / da0, da1 / da0, da2 / da0};
}

inline double analogResponseDb(const SectionSet& set, double f) {
    double w = 2 * kPi * f, db = 0;
    for (const auto& s : set) {
        if (!s.active) continue;
        double nr = s.n[0] - s.n[2] * w * w, ni = s.n[1] * w;
        double dr = s.d[0] - s.d[2] * w * w, di = s.d[1] * w;
        db += 10 * std::log10((nr * nr + ni * ni) / (dr * dr + di * di));
    }
    return db;
}

// A-weighted, pink-density average of the EQ gain (python: engine.loudness_comp_db)
inline double loudnessCompDb(const Params& p, int n = 400) {
    SectionSet set = design(p);
    double num = 0, den = 0;
    for (int i = 0; i < n; ++i) {
        double f = 30.0 * std::pow(16000.0 / 30.0, double(i) / (n - 1));
        double f2 = f * f;
        double ra = (std::pow(12194.0, 2) * f2 * f2) /
                    ((f2 + 20.6 * 20.6) * std::sqrt((f2 + 107.7 * 107.7) * (f2 + 737.9 * 737.9)) *
                     (f2 + 12194.0 * 12194.0));
        double w = ra * ra / f;
        num += w * std::pow(10.0, analogResponseDb(set, f) / 10);
        den += w;
    }
    return -10 * std::log10(num / den);
}

// ---- processing ---------------------------------------------------------------
struct BiquadState {            // transposed direct form II, double precision
    double z1 = 0, z2 = 0;
    inline double process(const Biquad& c, double x) {
        double y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return y;
    }
};

inline double tube(double x, double k, double b = kTubeBias) {
    if (k <= 0) return x;
    double t = std::tanh(b);
    return (std::tanh(k * x + b) - t) / ((1 - t * t) * k);
}

struct Transformer {            // LF flux-style soft saturation
    double lp = 0, a = 0, thr = 1;
    void prepare(double fs, double strength) {
        a = std::exp(-2 * kPi * kXfmrCornerHz / fs);
        thr = kXfmrThreshold / std::sqrt(strength > 1e-9 ? strength : 1e-9);
    }
    inline double process(double x) {
        lp = (1 - a) * x + a * lp;
        return (x - lp) + thr * std::tanh(lp / thr);
    }
};

}  // namespace pultec
