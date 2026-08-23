#ifndef ORBIDYN_CHAOS_HPP
#define ORBIDYN_CHAOS_HPP


#include "LowLevelOde.hpp"
#include "Integrators.hpp"
#include <odecraft/Chaos/VariationalSolvers.hpp>


namespace ode::python {

struct PyVarSolver : public PySolver{

    PyVarSolver(const py::object& f, const py::object& jac, const py::object& t0, const py::iterable& py_q0, const py::iterable& py_delta_q0, const py::object& period, const py::object& rtol, const py::object& atol, const py::object& min_step, const py::object& max_step, const py::object& stepsize, int dir, const py::iterable& py_args, const py::iterable& py_events, const std::string& scalar_type, const std::string& method);

    // TODO : should use something like `variational_integrator_t` to ensure that the variant is of
    // the correct type, but for now we just assume that the user knows what they are doing
    PyVarSolver(integrator_t solver, bool is_lowlevel);

    DEFAULT_RULE_OF_FOUR(PyVarSolver)

    py::object py_logksi() const;

    py::object py_lyap() const;

    py::object py_t_lyap() const;

    py::object py_delta_s() const;

    py::object copy() const override;

    template<typename T>
    chaos::ChaoticSolver<T, 0, UtilPolicy::RichVirtual>* cast();

    template<typename T>
    const chaos::ChaoticSolver<T, 0, UtilPolicy::RichVirtual>* cast() const;

};

class PyVarODE : public PyODE{

public:

    PyVarODE(const py::object& f, const py::object& jac, const py::object& t0, const py::iterable& q0, const py::iterable& delta_q0, const py::object& period, const py::object& rtol, const py::object& atol, const py::object& min_step, const py::object& max_step, const py::object& stepsize, int dir, const py::iterable& py_args, const py::iterable& py_events, const std::string& scalar_type, const py::str& method);

    DEFAULT_RULE_OF_FOUR(PyVarODE);

    template<typename T>
    chaos::VariationalODE<T, 0>& varode();

    template<typename T>
    const chaos::VariationalODE<T, 0>& varode() const;

    py::object py_t_lyap() const;

    py::object py_lyap() const;

    py::object py_kicks() const;

    py::object copy() const override;
    
};


} // namespace ode::python

#endif // ORBIDYN_CHAOS_HPP