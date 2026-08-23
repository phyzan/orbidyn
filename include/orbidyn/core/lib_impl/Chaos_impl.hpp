#ifndef ORBIDYN_CHAOS_IMPL_HPP
#define ORBIDYN_CHAOS_IMPL_HPP


#include <odecraft/Chaos/VariationalSolvers_impl.hpp>
#include "../lib/Chaos.hpp"


namespace ode::python {

template<typename T>
ode::chaos::ChaoticSolver<T, 0, UtilPolicy::RichVirtual>* PyVarSolver::cast(){
    pbox::owner<OdeRichSolver<T>>& ref = std::get<pbox::owner<OdeRichSolver<T>>>(this->integrator);
    return ref.template cast<ode::chaos::ChaoticSolver<T, 0, UtilPolicy::RichVirtual>>();
}

template<typename T>
const ode::chaos::ChaoticSolver<T, 0, UtilPolicy::RichVirtual>* PyVarSolver::cast() const {
    const pbox::owner<OdeRichSolver<T>>& ref = std::get<pbox::owner<OdeRichSolver<T>>>(this->integrator);
    return ref.template cast<ode::chaos::ChaoticSolver<T, 0, UtilPolicy::RichVirtual>>();
}

template<typename T>
ode::chaos::VariationalODE<T, 0>& PyVarODE::varode(){
    pbox::owner<ODE<T>>& ref = std::get<pbox::owner<ODE<T>>>(this->ode);
    return *ref.template cast<::ode::chaos::VariationalODE<T, 0>>();
}

template<typename T>
const ode::chaos::VariationalODE<T, 0>& PyVarODE::varode() const {
    const pbox::owner<ODE<T>>& ref = std::get<pbox::owner<ODE<T>>>(this->ode);
    return *ref.template cast<::ode::chaos::VariationalODE<T, 0>>();
}

} // namespace ode::python

#endif // ORBIDYN_CHAOS_IMPL_HPP