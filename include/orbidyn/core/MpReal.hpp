#ifndef ORBIDYN_MPREAL_HPP
#define ORBIDYN_MPREAL_HPP

/**
 * @file MpReal.hpp
 * @brief orbidyn's arbitrary-precision scalar: the one from the compiled interface.
 *
 * The only scalar in ORBIDYN_SCALARS that is not a builtin, and the only one whose identity
 * is decided by a build option: odecraft selects ode::crafted::mpreal_t with
 * ODECRAFT_USE_LAZY_MPREAL, between lazex::LazyType<mpfr::mpreal> and plain mpfr::mpreal.
 *
 * orbidyn does not pin that option -- it defaults it off and lets the caller override it
 * (CMAKE_ARGS="-DODECRAFT_USE_LAZY_MPREAL=ON" pip install .), so flipping the macro is the
 * single thing that has to change. Aliasing crafted::mpreal_t rather than spelling either
 * type out is what makes that true: there is no second definition to keep in step.
 *
 * Because the macro decides a type, every translation unit in the program has to be given
 * the same value for it -- including JIT-compiled user code, which is built outside CMake.
 * That is what OdeSystem.compile_flags() reads back out of _buildconfig.py.
 */

#include <odecraft/Compiled/Toolkit.hpp>

namespace ode::python {

using mpreal_t = ::ode::crafted::mpreal_t;

} // namespace ode::python

#endif // ORBIDYN_MPREAL_HPP
