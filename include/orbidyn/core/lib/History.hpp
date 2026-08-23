#ifndef ORBIDYN_HISTORY_HPP
#define ORBIDYN_HISTORY_HPP


#include <odecraft/OdeResult/OdeResult.hpp>
#include "Tools.hpp"


namespace ode::python {

template<typename... T>
using pyres_variant_t = std::variant<pbox::owner<OdeResult<T>>...>;

using result_t = pyres_variant_t<ORBIDYN_SCALARS>;

struct PyOdeResult : DtypeDispatcher{

    PyOdeResult(result_t result, const pyshape_t& state_shape);

    DEFAULT_RULE_OF_FOUR(PyOdeResult)
    
    virtual ~PyOdeResult() = default;

    py::object                  t() const;

    py::object                  q() const;

    py::tuple                   event_data(const py::str& event) const;

    py::bool_                   diverges() const;

    py::bool_                   success() const;

    py::float_                  runtime() const;

    py::str                     message() const;

    void                        examine() const;

    result_t history{};
    pyshape_t state_dims{};

};


struct PyOdeSolution : public PyOdeResult{

    // TODO : Should use somehting like `solution_t` to ensure that the variant is of the correct type,
    // but for now we just assume that the user knows what they are doing
    PyOdeSolution(result_t result, const pyshape_t& state_shape);

    DEFAULT_RULE_OF_FOUR(PyOdeSolution)

    py::object operator()(const py::object& t) const;

    py::object get_frame(const py::object& t) const;

    py::object get_array(const py::array& py_array) const;

    size_t nsys{};

};


} // namespace ode::python

#endif // ORBIDYN_HISTORY_HPP