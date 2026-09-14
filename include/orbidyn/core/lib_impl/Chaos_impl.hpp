#ifndef ORBIDYN_CHAOS_IMPL_HPP
#define ORBIDYN_CHAOS_IMPL_HPP


#include "../lib/Chaos.hpp"


namespace ode::python {

template<typename T>
ChaoticSolver<T>* PyVarSolver::cast(){
    pbox::owner<OdeRichSolver<T>>& ref = std::get<pbox::owner<OdeRichSolver<T>>>(this->integrator);
    return ref.template cast<ChaoticSolver<T>>();
}

template<typename T>
const ChaoticSolver<T>* PyVarSolver::cast() const {
    const pbox::owner<OdeRichSolver<T>>& ref = std::get<pbox::owner<OdeRichSolver<T>>>(this->integrator);
    return ref.template cast<ChaoticSolver<T>>();
}

template<typename T>
VariationalODE<T>& PyVarODE::varode(){
    pbox::owner<ODE<T>>& ref = std::get<pbox::owner<ODE<T>>>(this->ode);
    return *ref.template cast<VariationalODE<T>>();
}

template<typename T>
const VariationalODE<T>& PyVarODE::varode() const {
    const pbox::owner<ODE<T>>& ref = std::get<pbox::owner<ODE<T>>>(this->ode);
    return *ref.template cast<VariationalODE<T>>();
}

} // namespace ode::python

#endif // ORBIDYN_CHAOS_IMPL_HPP