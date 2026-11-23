// Checks cpp/dsp/PultecDsp.h against the Python reference and the same
// behavioural targets as python/tests. Usage: pultec_dsp_tests reference.txt
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "PultecDsp.h"

using namespace pultec;
static int fails = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++fails; std::printf("FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static double thdPercent(const std::vector<double>& y, int m) {
    int n = (int)y.size();
    auto amp = [&](int k) {
        double re = 0, im = 0;
        for (int i = 0; i < n; ++i) { double a = 2 * kPi * k * m * i / n; re += y[i] * std::cos(a); im += y[i] * std::sin(a); }
        return 2 * std::sqrt(re * re + im * im) / n;
    };
    double h1 = amp(1), s = 0;
    for (int k = 2; k <= 10; ++k) { double a = amp(k); s += a * a; }
    return 100 * std::sqrt(s) / h1;
}

int main(int argc, char** argv) {
    std::ifstream in(argc > 1 ? argv[1] : "cpp/reference/reference.txt");
    CHECK(in.good(), "cannot open reference file");
    std::string line; int nsec = 0, ntube = 0;
    double worst = 0, worstTube = 0;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream is(line); std::string tag; is >> tag;
        if (tag == "SECTIONS") {
            Params p; double fs; int n;
            is >> p.lfFreq >> p.lfBoost >> p.lfCut >> p.hfFreq >> p.hfBoost >> p.hfBw >> p.hfCutFreq >> p.hfCut >> fs >> n;
            std::vector<Biquad> ref(n);
            for (auto& b : ref) is >> b.b0 >> b.b1 >> b.b2 >> b.a1 >> b.a2;
            std::vector<Biquad> got;
            for (const auto& s : design(p)) if (s.active) got.push_back(toDigital(s, fs));
            CHECK((int)got.size() == n, "section count %zu vs %d", got.size(), n);
            if ((int)got.size() == n) for (int i = 0; i < n; ++i) {
                double e = std::fabs(got[i].b0 - ref[i].b0) + std::fabs(got[i].b1 - ref[i].b1) + std::fabs(got[i].b2 - ref[i].b2)
                         + std::fabs(got[i].a1 - ref[i].a1) + std::fabs(got[i].a2 - ref[i].a2);
                if (e > worst) worst = e;
                CHECK(e < 1e-8, "coeff mismatch fs=%g section %d err=%g", fs, i, e);
            }
            ++nsec;
        } else if (tag == "TUBE") {
            double k, x, y; is >> k >> x >> y;
            double e = std::fabs(tube(x, k) - y); if (e > worstTube) worstTube = e;
            CHECK(e < 1e-12, "tube mismatch k=%g x=%g", k, x);
            ++ntube;
        }
    }
    std::printf("coefficient sets: %d (worst abs err %.2e), tube samples: %d (worst %.2e)\n", nsec, worst, ntube, worstTube);
    CHECK(nsec > 10 && ntube > 10, "reference file looks empty");

    // behavioural targets (same as python/tests)
    {   Params p; p.lfFreq = 60; p.lfBoost = 2.5; p.lfCut = 2.5;
        SectionSet s = design(p);
        double lo = analogResponseDb(s, 10), mn = 1e9, fmn = 0;
        for (double f = 10; f < 24000; f *= 1.002) { double r = analogResponseDb(s, f); if (r < mn) { mn = r; fmn = f; } }
        CHECK(lo > 2.0 && lo < 4.0, "LF shelf %.2f dB", lo);
        CHECK(mn > -2.6 && mn < -1.4 && fmn > 150 && fmn < 600, "LF dip %.2f dB @ %.0f Hz", mn, fmn);
        std::printf("LF 2.5/2.5 @60: shelf %+.2f dB, dip %.2f dB @ %.0f Hz\n", lo, mn, fmn); }
    {   Params p; p.hfFreq = 5000; p.hfBoost = 10; p.hfBw = 0;
        double sharp = analogResponseDb(design(p), 5000); p.hfBw = 10;
        double broad = analogResponseDb(design(p), 5000);
        CHECK(std::fabs(sharp - 20) < 0.3 && std::fabs(sharp - broad - 9) < 0.3, "HF %.2f/%.2f", sharp, broad); }
    {   Params p; p.hfCut = 10; p.hfCutFreq = 10000;
        CHECK(std::fabs(analogResponseDb(design(p), 10000) + 12) < 0.3, "HF cut @sel"); }
    {   const int n = 1 << 15; const double fs = 192000; std::vector<double> y(n);
        for (int i = 0; i < n; ++i) y[i] = tube(std::pow(10.0, -12 / 20.0) * std::sin(2 * kPi * 171.0 * i / n), kTubeKCal);
        double thd = thdPercent(y, 171);
        std::printf("tube THD @ -12 dBFS: %.4f %%\n", thd);
        CHECK(std::fabs(thd - 0.10) < 0.01, "THD %.4f", thd);
        double t100 = 0, t1k = 0;
        for (int pass = 0; pass < 2; ++pass) {
            int m = pass ? 171 : 17; Transformer tr; tr.prepare(fs, 1.0);
            for (int i = 0; i < n; ++i) y[i] = tr.process(std::pow(10.0, -6 / 20.0) * std::sin(2 * kPi * m * (double)i / n));
            (pass ? t1k : t100) = thdPercent(y, m);
        }
        CHECK(t100 > 5 * t1k, "xfmr 100 Hz %.3f vs 1 kHz %.3f", t100, t1k); }
    std::printf(fails ? "%d FAILURES\n" : "all C++ checks passed\n", fails);
    return fails ? 1 : 0;
}
