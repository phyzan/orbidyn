#ifndef ORBIDYN_SUBINTEGRATORS_HPP
#define ORBIDYN_SUBINTEGRATORS_HPP


#include "Integrators.hpp"

namespace ode::python {


struct PyRK23 : public CRTPSolverLayer<PyRK23> {

    using Base = CRTPSolverLayer<PyRK23>;
    using Base::Base;

    static constexpr const char* method_name = "RK23";

    py::object copy() const override;

};


struct PyRK45 : public CRTPSolverLayer<PyRK45> {

    using Base = CRTPSolverLayer<PyRK45>;
    using Base::Base;

    static constexpr const char* method_name = "RK45";

    py::object copy() const override;

};


struct PyDOP853 : public CRTPSolverLayer<PyDOP853> {

    using Base = CRTPSolverLayer<PyDOP853>;
    using Base::Base;

    static constexpr const char* method_name = "DOP853";

    py::object copy() const override;

};


struct PyBDF : public CRTPSolverLayer<PyBDF> {

    using Base = CRTPSolverLayer<PyBDF>;
    using Base::Base;
    
    static constexpr const char* method_name = "BDF";

    py::object copy() const override;

};

struct PyRK4 : public CRTPSolverLayer<PyRK4> {

    using Base = CRTPSolverLayer<PyRK4>;
    using Base::Base;

    static constexpr const char* method_name = "RK4";

    py::object copy() const override;

};

} // namespace ode::python


#endif // ORBIDYN_SUBINTEGRATORS_HPP