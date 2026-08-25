#ifndef ORBIDYN_INTEGRATORS_HPP
#define ORBIDYN_INTEGRATORS_HPP


#include <odecraft/Core/VirtualBase.hpp>
#include "Tools.hpp"


#define ORBIDYN_VISIT_SOLVER_VARIANT(...) \
    std::visit([&]<typename T>(const pbox::owner<OdeRichSolver<T>>& solver){ \
        __VA_ARGS__ \
    }, this->integrator);

#define ORBIDYN_MODIFY_SOLVER_VARIANT(...) \
    std::visit([&]<typename T>(pbox::owner<OdeRichSolver<T>>& solver){ \
        __VA_ARGS__ \
    }, this->integrator);

namespace ode::python {

template<typename... T>
using solver_variant_t = std::variant<pbox::owner<OdeRichSolver<T>>...>;

using integrator_t = solver_variant_t<ORBIDYN_SCALARS>;

struct PyConstSolver : DtypeDispatcher {

    PyConstSolver(const py::object& f, const py::object& t0, const py::iterable& py_q0, const py::object& jac, const py::object& rtol, const py::object& atol, const py::object& min_step, const py::object& max_step, const py::object& stepsize, int dir, const py::iterable& py_args, const py::iterable& py_events, const std::string& scalar_type, const std::string& method);

    PyConstSolver(const std::string& scalar_type);

    PyConstSolver(integrator_t solver, bool is_lowlevel);

    DEFAULT_RULE_OF_FOUR(PyConstSolver)

    ~PyConstSolver() = default;

    void                init_scalar_type();

    py::object          t0() const;

    py::object          q0() const;

    int                 direction() const; 

    py::object          t() const;

    py::object          t_old() const;

    py::object          q() const;

    py::object          q_old() const;

    py::object          stepsize() const;

    py::object          diverges() const;

    py::object          is_dead() const;

    py::object          nsys() const;

    py::object          rhs_eval_count() const;

    py::object          jac_eval_count() const;

    void                show_state(int digits) const;

    py::object          py_rhs(const py::object& t, const py::iterable& py_q) const;

    py::object          py_jac(const py::object& t, const py::iterable& py_q) const;

    py::tuple           timeit_rhs(const py::object& t, const py::iterable& py_q) const;

    py::tuple           timeit_jac(const py::object& t, const py::iterable& py_q) const;

    bool                py_at_event(const py::object& event_name = py::none()) const;

    py::object          current_event() const;

    py::str             status() const;    

    virtual             py::object  copy() const;

    integrator_t    integrator;
    bool            is_lowlevel;
};


struct PySolver : public PyConstSolver {

    using PyConstSolver::PyConstSolver;

    PySolver(const PyConstSolver& other);

    DEFAULT_RULE_OF_FOUR(PySolver)

    py::object          advance();

    py::tuple           timeit_step();

    py::object          advance_to_event(const py::object& events);

    py::object          advance_until(const py::object& time, const py::object& observer, const py::object& extra_steps);

    void                reset();

    bool                set_ics(const py::object& t0, const py::iterable& py_q0, const py::object& dt, int direction);

    void                kill(py::str reason);   
};


template<typename Solver>
struct CRTPSolverLayer : public PySolver {

    // exposes a common constructor for all exposed python solver classes, to be used in the bindings
    // RK45 and other solvers must derive from it.

    static constexpr const char* method = Solver::method_name;

    CRTPSolverLayer(integrator_t solver, bool is_lowlevel) : PySolver(std::move(solver), is_lowlevel) {}

    CRTPSolverLayer(const py::object& f, const py::object& t0, const py::iterable& py_q0, const py::object& jac, const py::object& rtol, const py::object& atol, const py::object& min_step, const py::object& max_step, const py::object& stepsize, int dir, const py::iterable& py_args, const py::iterable& py_events, const std::string& scalar_type)
        : PySolver(f, t0, py_q0, jac, rtol, atol, min_step, max_step, stepsize, dir, py_args, py_events, scalar_type, method) {}
    
};


struct AbstractIntegrator : public PySolver {

    /*
    A class that exposes the constructor of PySolver that takes as argument
    the integration method name, to be overriden from other Python classes.
    */

    using PySolver::PySolver;
};

template<typename T, bool force_jac, typename Callable>
bool init_ode_data(Callable&& action, const py::object& py_rhs, const py::object& py_jac, const pyshape_t& state_shape, const py::iterable& py_args, const py::iterable& py_events);

// func::template operator()<T>(OdeRichSolver<T>* solver)
template<typename Callable>
void py_advance_all_general(py::object& list, Callable&& func, int threads, bool display_progress);


void py_advance_all(py::object& list, double t_goal, int threads, bool display_progress);

void py_advance_all_to_event(py::object& list, const py::object& events, double tmax, int threads, bool display_progress);

} // namespace ode::python


#endif // ORBIDYN_INTEGRATORS_HPP