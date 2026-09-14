#ifndef ORBIDYN_CRAFTED_HPP
#define ORBIDYN_CRAFTED_HPP

/**
 * @file Crafted.hpp
 * @brief The odecraft surface orbidyn is built on, imported into ode::python.
 *
 * orbidyn uses the pre-compiled `ode::crafted` interface exclusively: every template
 * parameter but the scalar type is already pinned there (dynamic system size, type-erased
 * callables, rich-virtual solvers), and the instantiations ship in libodecraft_crafted.a.
 * So no orbidyn translation unit instantiates a solver, and none of them may include an
 * odecraft `_impl.hpp` -- doing so would compile here what the library already provides.
 *
 * The imports below are using-*declarations*, not `using namespace`. That matters:
 * ode::python is nested inside ode, so the header-only `ode::ODE`, `ode::OdeResult`,
 * `ode::RK45` and friends are already visible by unqualified lookup here. A using-directive
 * would make every one of those names ambiguous; a using-declaration puts the name in
 * ode::python itself, where lookup finds it and stops. Unqualified `OdeResult<T>` in orbidyn
 * therefore means `ode::crafted::OdeResult<T>`, which is the whole point.
 *
 * Anything the library does not compile has no business in this header. If a name is
 * missing, it belongs in odecraft's Compiled/ headers first.
 */

#include <odecraft/Compiled/odecraft.hpp>
#include "MpReal.hpp" // IWYU pragma: keep

namespace ode::python {

// ---------------------------------------------------------------------------
// Scalars, callables and the one system type
// ---------------------------------------------------------------------------

using crafted::set_mpreal_prec,
      crafted::get_default_prec;

// crafted::ode_t is deliberately not imported: ode::python::ode_t is the variant of ODE
// handles (see lib/LowLevelOde.hpp). Spell the system type crafted::ode_t<T>.
using crafted::rhs_t,
      crafted::objfun_t,
      crafted::interp_t,
      crafted::observer_t,
      crafted::OdeData;

using crafted::Stepper,
      crafted::StepResult,
      crafted::JacPolicy,
      crafted::RootPolicy,
      crafted::Clock,
      crafted::State;

// ---------------------------------------------------------------------------
// Solvers
// ---------------------------------------------------------------------------

using crafted::OdeSolver,
      crafted::OdeRichSolver,
      crafted::BoxedRichSolver,
      crafted::BoxedInterp,
      crafted::make_rich_vsolver;

using crafted::Euler,
      crafted::RK4,
      crafted::RK23,
      crafted::RK45,
      crafted::DOP853,
      crafted::BDF;

// ---------------------------------------------------------------------------
// Driver and history
// ---------------------------------------------------------------------------

// crafted::ODE and crafted::VariationalODE are constructor wrappers and are used qualified,
// at the two places that build one. What gets held and passed around is OdeDriver -- the
// polymorphic base both of them are -- which lib/LowLevelOde.hpp names ODE<T> here.
using crafted::OdeDriver,
      crafted::EventCounter,
      crafted::OdeResult,
      crafted::OdeSolution,
      crafted::OrbitData;

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

using crafted::Event,
      crafted::BoxedEvent,
      crafted::EventList,
      crafted::EventOptions,
      crafted::EventPolicy,
      crafted::EventCollection,
      crafted::EventState,
      crafted::MaskedState,
      crafted::PreciseEvent,
      crafted::PeriodicEvent,
      crafted::make_event_list,
      crafted::make_precise_event,
      crafted::make_periodic_event;

// ---------------------------------------------------------------------------
// Dense output
// ---------------------------------------------------------------------------

using crafted::Interval,
      crafted::Interpolator,
      crafted::LocalInterpolator,
      crafted::LinkedInterpolator,
      crafted::InterpObj;

// ---------------------------------------------------------------------------
// Field interpolation. Not templated on the scalar: these are `double` throughout,
// with the dimension dynamic. See Compiled/NdInterpolators.hpp.
// ---------------------------------------------------------------------------

using crafted::field_values_t,
      crafted::grid_axes_t,
      crafted::CoordType;

using crafted::VirtualNdInterpolator,
      crafted::VirtualVectorField,
      crafted::RegularGrid,
      crafted::RegularGridInterpolator,
      crafted::RegularVectorField,
      crafted::DelaunayTri,
      crafted::TriPtr,
      crafted::ScatteredNdInterpolator,
      crafted::ScatteredVectorField;

// ---------------------------------------------------------------------------
// Variational integration
// ---------------------------------------------------------------------------

using crafted::ChaoticSolver,
      crafted::BoxedChaoticSolver,
      crafted::make_variational_solver,
      crafted::VariationalODE;

} // namespace ode::python

#endif // ORBIDYN_CRAFTED_HPP
