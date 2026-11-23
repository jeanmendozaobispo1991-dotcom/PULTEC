from dataclasses import dataclass


@dataclass
class Params:
    # Low band (knobs 0..10 like the hardware)
    lf_freq: int = 60
    lf_boost: float = 0.0
    lf_cut: float = 0.0
    # High boost
    hf_freq: int = 10000
    hf_boost: float = 0.0
    hf_bw: float = 5.0          # 0 = sharp, 10 = broad
    # High cut
    hf_cut_freq: int = 10000
    hf_cut: float = 0.0
    # Chain
    eq_in: bool = True          # hardware "IN": bypasses EQ network, amp stays
    input_gain_db: float = 0.0
    saturation: float = 1.0     # 0 = off, 1 = calibrated to hardware spec
    oversampling: int = 4
    # Behavioural LF model: provisional fit (docs/LF_MODEL.md)
    lf_boost_q: float = 0.563
    lf_boost_taper: float = 0.346
    lf_cut_taper: float = 0.757
    lf_cut_split: float = 0.638     # share of cut depth in the lower shelf
    lf_cut_k1: float = 0.673        # cut corner multipliers (x selector)
    lf_cut_k2: float = 11.76
