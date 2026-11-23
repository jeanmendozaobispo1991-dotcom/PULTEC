"""Numeric AC solver for R/L/C networks (modified nodal analysis).

Use it to validate the circuit once real schematic values are available:
    elements = [('R', 'in', 'a', 600), ('L', 'a', 'b', 31.4e-3), ...]
Node 0 / 'gnd' is ground. The source is a 1 V source behind Rs on node n_in.
"""
import numpy as np
from .constants import HF_BOOST_LC


def ac_response(elements, n_in, n_out, rs, freqs):
    allnodes = {n for _, a, b, _ in elements for n in (a, b)} | {n_in, n_out}
    nodes = sorted(allnodes - {0, 'gnd'}, key=str)
    ix = {n: i for i, n in enumerate(nodes)}
    out = np.empty(len(freqs), dtype=complex)
    for k, f in enumerate(freqs):
        s = 2j * np.pi * f
        Y = np.zeros((len(nodes), len(nodes)), dtype=complex)
        I = np.zeros(len(nodes), dtype=complex)
        for t, a, b, v in list(elements) + [('R', n_in, 'gnd', rs)]:
            y = {'R': 1 / v, 'C': s * v, 'L': 1 / (s * v)}[t]
            for p, q, g in ((a, a, 1), (b, b, 1), (a, b, -1), (b, a, -1)):
                if p in ix and q in ix:
                    Y[ix[p], ix[q]] += g * y
        I[ix[n_in]] = 1 / rs
        out[k] = np.linalg.solve(Y, I)[ix[n_out]]
    return out


def hf_tap_resonance_hz():
    """f0 = 1/(2 pi sqrt(LC)) for each published inductor/capacitor tap."""
    return {f: 1 / (2 * np.pi * np.sqrt(L * C)) for f, (L, C) in HF_BOOST_LC.items()}


def hf_tap_impedance():
    """sqrt(L/C) per tap: Q = sqrt(L/C)/R_total for a series resonator."""
    return {f: np.sqrt(L / C) for f, (L, C) in HF_BOOST_LC.items()}
