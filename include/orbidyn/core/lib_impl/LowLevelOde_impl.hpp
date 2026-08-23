#ifndef ORBIDYN_LOWLEVELODE_IMPL_HPP
#define ORBIDYN_LOWLEVELODE_IMPL_HPP


#include <odecraft/DenseOde/OdeInt_impl.hpp>
#include <odecraft/Interpolation/Univariate/StateInterp_impl.hpp>
#include "../lib/LowLevelOde.hpp"


namespace ode::python {

template<typename T, hasRhsFunc<T> OdeType>
PyODE::PyODE(OdeType ode, T t0, View1D<T> q0, T rtol, T atol, T min_step, T max_step, T stepsize, int dir, EventList<T> events, Integrator method) : DtypeDispatcher(get_scalar_type<T>()){
    this->is_lowlevel = true;
    this->state_dims = {py::ssize_t(q0.size())};

    this->ode = pbox::make_box<ODE<T>>(std::move(ode), t0, q0, q0.size(), rtol, atol, min_step, max_step, stepsize, dir, std::move(events), method);
}


} // namespace ode::python

#endif // ORBIDYN_LOWLEVELODE_IMPL_HPP