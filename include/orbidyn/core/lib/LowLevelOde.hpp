#ifndef ORBIDYN_LOWLEVELODE_HPP
#define ORBIDYN_LOWLEVELODE_HPP


#include <odecraft/DenseOde/OdeInt.hpp>
#include "Tools.hpp"


#define ORBIDYN_VISIT_ODE_VARIANT(...) \
    std::visit([&]<typename T>(const pbox::owner<ODE<T>>& ode_ptr){ \
        __VA_ARGS__ \
    }, this->ode);

#define ORBIDYN_MODIFY_ODE_VARIANT(...) \
    std::visit([&]<typename T>(pbox::owner<ODE<T>>& ode_ptr){ \
        __VA_ARGS__ \
    }, this->ode);


namespace ode::python {

template<typename... T>
using ode_variant_t = std::variant<pbox::owner<ODE<T>>...>;

using ode_t = ode_variant_t<ORBIDYN_SCALARS>;

class PyODE : public DtypeDispatcher{

public:
    
    PyODE(const py::object& f, const py::object& t0, const py::iterable& py_q0, const py::object& jacobian, const py::object& rtol, const py::object& atol, const py::object& min_step, const py::object& max_step, const py::object& stepsize, int dir, const py::iterable& py_args, const py::iterable& py_events, const py::str& method, const std::string& scalar_type);

    template<typename T, hasRhsFunc<T> OdeType>
    PyODE(OdeType ode, T t0, View1D<T> q0, T rtol, T atol, T min_step, T max_step, T stepsize, int dir, EventList<T> events, Stepper method);

    PyODE(ode_t ode_owner, pyshape_t state_shape, bool ode_is_lowlevel);

protected:

    PyODE(const std::string& scalar_type); //derived classes manage ode and state_dims creation

    void init_scalar_type();

public:

    virtual ~PyODE() = default;

    py::object call_Rhs(const py::object& t, const py::iterable& py_q) const;

    py::object call_Jac(const py::object& t, const py::iterable& py_q) const;

    py::object py_integrate(const py::object& interval, const py::object& t_eval, const py::iterable& event_options, int max_prints);

    py::object py_rich_integrate(const py::object& interval, const py::iterable& event_options, int max_prints);

    py::object py_integrate_until(const py::object& t, const py::object& t_eval, const py::iterable& event_options, int max_prints);

    py::object t_array() const;

    py::object q_array() const;

    py::tuple event_data(const py::str& event) const;

    virtual py::object copy() const;

    py::object solver_copy() const;

    py::object Nsys() const;

    py::object runtime() const;

    py::object diverges() const;

    py::object is_dead() const;

    void reset();

    void clear();

    ode_t ode{};
    pyshape_t state_dims{};
    bool is_lowlevel{};
};


void py_integrate_all(py::object& list, double interval, const py::object& t_eval, const py::iterable& event_options, int threads, bool display_progress);

} // namespace ode::python

#endif // ORBIDYN_LOWLEVELODE_HPP