#include <orbidyn/core/lib/SubIntegrators.hpp>
#include <orbidyn/core/lib/Tools.hpp>

using namespace ode::python;


// =================== Templated binding function ==================

template<typename DerivedSolver>
py::class_<DerivedSolver, PySolver> bind_stepper(py::module_& m, const char* name){
    return
    py::class_<DerivedSolver, PySolver>(m, name)
        .def(py::init<DerivedSolver>(), py::arg("solver"))
        .def(py::init<py::object, py::object, py::iterable, py::object, py::object, py::object, py::object, py::object, py::object, int, py::iterable, py::iterable, std::string>(),
            py::arg("f"),
            py::arg("t0"),
            py::arg("q0"),
            py::kw_only(),
            py::arg("jac")=py::none(),
            py::arg("rtol")=1e-6,
            py::arg("atol")=1e-12,
            py::arg("min_step")=0.,
            py::arg("max_step")=py::none(),
            py::arg("stepsize")=0.,
            py::arg("direction")=1,
            py::arg("args")=py::tuple(),
            py::arg("events")=py::tuple(),
            py::arg("scalar_type")="double"
    );
}

PYBIND11_MODULE(integrators, m) {

py::class_<PyFuncWrapper>(m, "LowLevelFunction")
    .def(py::init<py::capsule, py::ssize_t, py::array_t<py::ssize_t>, py::ssize_t, py::str>(),
        py::arg("pointer"),
        py::arg("input_size"),
        py::arg("output_shape"),
        py::arg("Nargs"),
        py::arg("scalar_type")="double")
    .def("__call__", &PyFuncWrapper::call, py::arg("t"), py::arg("q"))
    .def_property_readonly("scalar_type", [](const PyFuncWrapper& self){return getScalarTypeName(self.scalar_type);});


// ================= Main abstract interface classes ==========================
py::class_<PyConstSolver>(m, "OdeSolverView")
    .def_property_readonly("t0", &PyConstSolver::t0)
    .def_property_readonly("q0", &PyConstSolver::q0)
    .def_property_readonly("direction", &PyConstSolver::direction)
    .def_property_readonly("t", &PyConstSolver::t)
    .def_property_readonly("q", &PyConstSolver::q)
    .def_property_readonly("t_old", &PyConstSolver::t_old)
    .def_property_readonly("q_old", &PyConstSolver::q_old)
    .def_property_readonly("stepsize", &PyConstSolver::stepsize)
    .def_property_readonly("diverges", &PyConstSolver::diverges)
    .def_property_readonly("is_dead", &PyConstSolver::is_dead)
    .def_property_readonly("nsys", &PyConstSolver::nsys)
    .def_property_readonly("rhs_eval_count", &PyConstSolver::rhs_eval_count)
    .def_property_readonly("jac_eval_count", &PyConstSolver::jac_eval_count)
    .def_property_readonly("status", &PyConstSolver::status)
    .def_property_readonly("current_event", &PyConstSolver::current_event)
    .def("at_event", &PyConstSolver::py_at_event, py::arg("event_name")=py::none())
    .def("show_state", &PyConstSolver::show_state,
        py::arg("digits") = 8
    )
    .def("rhs", &PyConstSolver::py_rhs, py::arg("t"), py::arg("q"))
    .def("jac", &PyConstSolver::py_jac, py::arg("t"), py::arg("q"))
    .def("timeit_rhs", &PyConstSolver::timeit_rhs, py::arg("t"), py::arg("q"))
    .def("timeit_jac", &PyConstSolver::timeit_jac, py::arg("t"), py::arg("q"))
    .def("copy", &PyConstSolver::copy)
    .def_property_readonly("scalar_type", [](const PyConstSolver& self){return getScalarTypeName(self.scalar_type);});

py::class_<PySolver, PyConstSolver>(m, "OdeSolver")
    .def("advance", &PySolver::advance)
    .def("timeit_step", &PySolver::timeit_step)
    .def("advance_to_event", &PySolver::advance_to_event, py::arg("events")=py::none())
    .def("advance_until", &PySolver::advance_until, py::arg("t"), py::arg("observer")=py::none(), py::arg("extra_steps")=py::none())
    .def("reset", &PySolver::reset)
    .def("set_ics", &PySolver::set_ics, py::arg("t0"), py::arg("q0"), py::arg("stepsize")=0, py::arg("direction")=0)
    .def("kill", &PySolver::kill, py::arg("reason"));

// ================= Solver with exposed constructor to override in Python ==========================
py::class_<AbstractIntegrator, PySolver>(m, "AbstractIntegrator")
    .def(py::init<PyConstSolver>(), py::arg("solver"))
    .def(py::init<py::object, py::object, py::iterable, py::object, py::object, py::object, py::object, py::object, py::object, int, py::iterable, py::iterable, std::string, std::string>(),
        py::arg("f"),
        py::arg("t0"),
        py::arg("q0"),
        py::kw_only(),
        py::arg("jac")=py::none(),
        py::arg("rtol")=1e-6,
        py::arg("atol")=1e-12,
        py::arg("min_step")=0.,
        py::arg("max_step")=py::none(),
        py::arg("stepsize")=0.,
        py::arg("direction")=1,
        py::arg("args")=py::tuple(),
        py::arg("events")=py::tuple(),
        py::arg("scalar_type")="double",
        py::arg("method")="RK45"
    );


bind_stepper<PyRK23>(m, "RK23");
bind_stepper<PyRK45>(m, "RK45");
bind_stepper<PyDOP853>(m, "DOP853");
bind_stepper<PyBDF>(m, "BDF");
bind_stepper<PyRK4>(m, "RK4");

m.def("advance_all", &py_advance_all, py::arg("solvers"), py::arg("t_goal"), py::arg("threads")=-1, py::arg("display_progress")=false);

m.def("advance_all_to_event", &py_advance_all_to_event, py::arg("solvers"), py::arg("events"), py::arg("tmax"), py::arg("threads")=-1, py::arg("display_progress")=false);

m.def("set_mpreal_prec",
    mpfr::mpreal::set_default_prec,
    py::arg("bits"),
    "Set the default MPFR precision (in bits) for mpfr::mpreal.")
.def("mpreal_prec", &mpfr::mpreal::get_default_prec);

} // PYBIND11_MODULE(integrators, m)
