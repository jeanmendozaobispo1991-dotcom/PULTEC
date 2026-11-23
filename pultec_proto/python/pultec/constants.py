"""Spec values and where each one comes from.

[source]  taken from a published manual/measurement
[derived] deduced from sources, to be verified
[assumed] engineering decision, to be tuned by ear/measurement
"""

LF_FREQS_HZ = (20, 30, 60, 100)
HF_BOOST_FREQS_HZ = (3000, 4000, 5000, 8000, 10000, 12000, 16000)
HF_CUT_FREQS_HZ = (5000, 10000, 20000)

LF_BOOST_MAX_DB = 16.0     # Manley + SMC2024 paper (MusicTech: 13.5)          [source, disputed]
LF_CUT_MAX_DB = 20.0       # Manley + paper (MusicTech 17.5, Waves 18)         [source, disputed]
HF_BOOST_MAX_DB = 20.0     # Manley panel + Waves (MusicTech: 18)              [source]
HF_BW_SPREAD_DB = 9.0      # SOS 2019 Audio Precision: narrow vs broad @ max   [source]
HF_CUT_MAX_DB = 21.0       # Manley (paper 20, MusicTech 16)                   [source, disputed]
HF_CUT_AT_SEL_FRAC = 12.0 / 21.0   # Manley: -12 dB at selected f, -21 max     [source]

LF_CUT_CORNER_OCT_ABOVE = 0.5      # SOS 2019: cut corners ~half octave above  [source]

INSERTION_LOSS_DB = 16.0   # paper SMC2024, AudioTechnology, GearNews           [source]
SOURCE_LOAD_OHMS = 600.0   # SOS 2019: nominal 600 ohm EQ interface             [source]

THD_SPEC_PERCENT = 0.15    # at +10 dBm, original spec                          [source]
REF_DBFS = -18.0           # -18 dBFS == +4 dBu (0 VU)                          [assumed]
REF_DBU = 4.0

# HF boost inductor taps: VintageWindings revised values (not original Pultec)  [derived]
HF_BOOST_LC = {
    16000: (29.9e-3, 3.3e-9),
    12000: (31.4e-3, 5.6e-9),
    10000: (31.4e-3, 8.2e-9),
    8000:  (48.2e-3, 8.2e-9),
    5000:  (67.5e-3, 15e-9),
    4000:  (87.9e-3, 18e-9),
}
