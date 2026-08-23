#ifndef ORBIDYN_HPP
#define ORBIDYN_HPP

/**
 * @file orbidyn.hpp
 * @brief Umbrella header: every public orbidyn declaration plus every template definition.
 *
 * This is the entry point for JIT-compiled user code (see OdeSystem.compile_flags() in
 * python/orbidyn/symbolic.py), which has no way of knowing which subset it needs. The
 * library's own translation units should NOT use it - they include the specific
 * lib/ and lib_impl/ headers they need, so that touching one implementation header does
 * not force a rebuild of every static library.
 *
 * Each header under lib_impl already pulls in its matching lib header, so only the ones with
 * no _impl counterpart are listed separately.
 */

#include <orbidyn/core/pycast/pycast.hpp>
#include <orbidyn/core/Dispatcher.hpp>

#include <orbidyn/core/lib/EventHandling.hpp>
#include <orbidyn/core/lib/History.hpp>
#include <orbidyn/core/lib/SubIntegrators.hpp>
#include <orbidyn/core/lib/Spatial.hpp>
#include <orbidyn/core/lib/Sampling.hpp>

#include <orbidyn/core/lib_impl/Tools_impl.hpp>
#include <orbidyn/core/lib_impl/Integrators_impl.hpp>
#include <orbidyn/core/lib_impl/LowLevelOde_impl.hpp>
#include <orbidyn/core/lib_impl/Chaos_impl.hpp>

#endif // ORBIDYN_HPP
