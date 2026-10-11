from . import lowlevelode, spatial, sampling, integrators, eventhandling, chaos, history # type: ignore
from .lowlevelode import * #type: ignore
from .spatial import * #type: ignore
from .sampling import * #type: ignore
from .integrators import * #type: ignore
from .eventhandling import * #type: ignore
from .chaos import * #type: ignore
from .history import * #type: ignore
from .symbolic import *
from .interpolate import *


# Every extension module carries its own copy of odecraft, and with it its own scratch buffers
# for mpreal arithmetic; each module's _set_mpreal_prec only resizes its own. A new extension
# module must be added here.
_EXTENSION_MODULES = (lowlevelode, spatial, sampling, integrators, eventhandling, chaos, history)


def set_mpreal_prec(bits: int) -> None:
    """
    Set the precision for MPFR arbitrary-precision arithmetic.

    Sets the bit precision used by the "mpreal" scalar type (arbitrary precision
    floating-point numbers via MPFR library). This precision applies to all new
    mpreal objects created after this call.

    The default precision is 53 bits, which is equivalent to IEEE 754 double
    precision.

    Parameters
    ----------
    bits : int
        Number of bits of precision. Examples:
        - 53 (default): Equivalent to double precision (Python's float)
        - 128: ~38 decimal digits
        - 256: ~77 decimal digits
        - 1024: ~308 decimal digits

    Notes
    -----
    Higher precision costs more memory and computation time. Set only as high as
    needed for your application.

    As in MPFR itself, the precision belongs to the calling thread. The parallel
    functions (advance_all, advance_all_to_event, integrate_all) carry it over to
    the threads they start.

    Examples
    --------
    >>> set_mpreal_prec(128)  # Set to 128-bit precision
    >>> ode = LowLevelODE(f, 0, q0, scalar_type="mpreal")
    >>> # Subsequent ODEs will use 128-bit precision
    """
    for module in _EXTENSION_MODULES:
        module._set_mpreal_prec(bits)
