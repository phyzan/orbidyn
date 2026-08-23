<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++20">
  <img src="https://img.shields.io/badge/Python-3.10%2B-yellow?style=for-the-badge&logo=python&logoColor=white" alt="Python 3.10+">
  <img src="https://img.shields.io/badge/License-MIT-purple?style=for-the-badge" alt="MIT License">
</p>

<h1 align="center">OrbiDyn</h1>

<p align="center">
  <strong>A Python package for high-performance ODE solvers</strong>
</p>

---


# Overview

OrbiDyn is a Python package that provides high-performance Ordinary Differential Equation (ODE) solvers by compiling C++ code from [OdeCraft](https://github.com/phyzan/odecraft) and exposing it through a Python interface using [pybind11](https://github.com/pybind/pybind11).

ODE systems can be defined directly in Python using the symbolic tools provided by [NumiPhy](https://github.com/phyzan/numiphy). These systems are automatically compiled to machine code and passed to the compiled backend for efficient numerical integration.

The library aims to provide a clean, familiar interface similar to [SciPy](https://docs.scipy.org/doc/scipy/reference/generated/scipy.integrate.odeint.html)'s odeint, while delivering significantly higher performance, particularly for small systems of ODEs.

# Installation

## Requirements

A C++20 compiler, CMake 3.16+, and the MPFR/GMP libraries are required.

**Ubuntu/Debian:**
```bash
sudo apt install build-essential
sudo apt install cmake
sudo apt install libmpfr-dev libgmp-dev
```

**macOS:**
```bash
xcode-select --install
brew install cmake
brew install mpfr gmp
```

**Windows (MSYS2/MinGW):**
```bash
pacman -S mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-mpfr mingw-w64-x86_64-gmp
```

## Initialization

After installing the dependencies, initialize the submodules:

```bash
git submodule update --init --recursive
```

Then, install the package using pip:

```bash
pip install .
```


# Features

Similar to [OdeCraft](https://github.com/phyzan/odecraft), OrbiDyn provides a very similar interface and tools:

- Flexible numerical type support. Solve ODEs using:
  - `float` (float32)
  - `double` precision (standard, float64)
  - `long double` precision (float80)
  - `mpreal` for arbitrary precision
- Integration with event detection and handling
- Support for dense output and interpolation
- Efficient integrators that advance the solution in-place without storing the full trajectory
- Classes that store integration history
- Support for adaptive step size control
- Support for both stiff and non-stiff ODE systems


## Quick example

```python
from orbidyn import *

t, x, y = symbols('t, x, y')

event = SymbolicPreciseEvent("event", y-1) # detects when y crosses 1

dq_dt = [y, -x]
odesys = OdeSystem(dq_dt, t, [x, y], events=[event])

ode = odesys.get(t0=0, q0=[3.0, 0.0], rtol=1e-6, atol=1e-9, stepsize=0.01, compiled=True, scalar_type="double") #use compiled=False for pure python version

solver = ode.solver()   # get a copy of the internal solver
print(ode.__class__)    #LowLevelODE
print(solver.__class__) #RK45

while not solver.at_event():
    solver.advance()

print(f"Event detected at t = {solver.t}")
print("State at detected event:", solver.q)
print("Expected state:", "[..., 1. ]")
```