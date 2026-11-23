"""Pultec EQP-1A style prototype (offline, NumPy/SciPy). See README.md."""
from .params import Params
from .engine import process, eq_response_db
__all__ = ["Params", "process", "eq_response_db"]
