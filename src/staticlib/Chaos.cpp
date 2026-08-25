#include <orbidyn/core/lib_impl/Chaos_impl.hpp>
#include <orbidyn/core/lib_impl/LowLevelOde_impl.hpp>
#include <orbidyn/core/lib_impl/Integrators_impl.hpp>
#include <orbidyn/core/lib_impl/Tools_impl.hpp>



namespace ode::python {

//===========================================================================================
//                                      PyVarSolver
//===========================================================================================


PyVarSolver::PyVarSolver(const py::object& f, const py::object& jac, const py::object& t0, const py::iterable& q0_main, const py::iterable& delta_q0, const py::object& period, const py::object& rtol, const py::object& atol, const py::object& min_step, const py::object& max_step, const py::object& stepsize, int dir, const py::iterable& py_args, const py::iterable& py_events, const std::string& scalar_type, const std::string& method) : PySolver(scalar_type) {

    dispatch_scalar_type<void>(this->scalar_type,
        [&]<typename T>(){

            size_t nsys = size_t(py::len(q0_main));

            if (jac.is_none()){
                throw py::value_error("Variational solvers require an exact jacobian for the original system");
            } else if (size_t(py::len(delta_q0)) != nsys){
                throw py::value_error("The variational state vector delta_q0 must have the same size as q0");
            }

            // ------- fill the initial state vector with q0 and delta_q0 -------
            std::vector<T> vector(2*nsys);
            pass_values(vector.data(), q0_main, nsys);
            pass_values(vector.data()+nsys, delta_q0, nsys);
            // ----------------------------------------------------------
            pyshape_t main_shape_state = {py::ssize_t(nsys)};
            this->is_lowlevel = init_ode_data<T, true>([&](auto ode_obj, EventList<T>&& events){
                this->integrator = ode::chaos::make_variational_solver<UtilPolicy::RichVirtual>(getIntegrator(method), std::move(ode_obj), t0.cast<T>(), View1D{vector.data(), nsys}, View1D{vector.data()+nsys, nsys}, period.cast<T>(), rtol.cast<T>(), atol.cast<T>(), min_step.cast<T>(), (max_step.is_none() ? 0 : max_step.cast<T>()), stepsize.cast<T>(), dir, std::move(events));
            }, f, jac, main_shape_state, py_args, py_events);
        }
    );
}

PyVarSolver::PyVarSolver(integrator_t solver, bool is_lowlevel) : PySolver(std::move(solver), is_lowlevel) {}

py::object PyVarSolver::py_logksi() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(this->cast<T>()->get_log_ksi());
    )
}

py::object PyVarSolver::py_lyap() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(this->cast<T>()->get_lyapunov_exponent());
    )
}

py::object PyVarSolver::py_t_lyap() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(this->cast<T>()->get_elapsed_time());
    )
}

py::object PyVarSolver::py_delta_s() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(this->cast<T>()->get_stretching_number());
    )
}

py::object PyVarSolver::copy() const{
    return py::cast(PyVarSolver(*this));
}




//===========================================================================================
//                                      PyVarODE
//===========================================================================================


PyVarODE::PyVarODE(const py::object& f, const py::object& jac, const py::object& t0, const py::iterable& q0_main, const py::iterable& delta_q0, const py::object& period, const py::object& rtol, const py::object& atol, const py::object& min_step, const py::object& max_step, const py::object& stepsize, int dir, const py::iterable& py_args, const py::iterable& py_events, const std::string& scalar_type, const py::str& method) : PyODE(scalar_type){
    dispatch_scalar_type<void>(this->scalar_type,
        [&]<typename T>(){
            size_t nsys = size_t(py::len(q0_main));

            if (jac.is_none()){
                throw py::value_error("Variational solvers require an exact jacobian for the original system");
            } else if (size_t(py::len(delta_q0)) != nsys){
                throw py::value_error("The variational state vector delta_q0 must have the same size as q0");
            }

            // ------- fill the initial state vector with q0 and delta_q0 -------
            std::vector<T> vector(2*nsys);
            pass_values(vector.data(), q0_main, nsys);
            pass_values(vector.data()+nsys, delta_q0, nsys);
            // ----------------------------------------------------------
            pyshape_t full_shape_state = {py::ssize_t(2*nsys)};
            pyshape_t main_shape_state = {py::ssize_t(nsys)};
            this->state_dims = std::move(full_shape_state);
            this->is_lowlevel = init_ode_data<T, true>([&](auto ode_obj, EventList<T>&& events){
                this->ode = pbox::make_box<::ode::chaos::VariationalODE<T, 0>>(
                    std::move(ode_obj), py::cast<T>(t0), View1D{vector.data(), nsys}, View1D{vector.data()+nsys, nsys}, py::cast<T>(period), py::cast<T>(rtol), py::cast<T>(atol), py::cast<T>(min_step), (max_step.is_none() ? 0 : max_step.cast<T>()), py::cast<T>(stepsize), dir, std::move(events), getIntegrator(method)
                );
                }, f, jac, main_shape_state, py_args, py_events
            );
        }
    );
}

py::object PyVarODE::py_t_lyap() const{
    return ORBIDYN_VISIT_ODE_VARIANT(
        const ode::chaos::VariationalODE<T, 0>& vode = varode<T>();
        View<T> res(vode.renorm_times().data(), vode.renorm_times().size());
        return py::cast(res);
    );
}

py::object PyVarODE::py_lyap() const{

    return ORBIDYN_VISIT_ODE_VARIANT(
        const ode::chaos::VariationalODE<T, 0>& vode = varode<T>();
        View<T> res(vode.lyap_values().data(), vode.lyap_values().size());
        return py::cast(res);
    )
}

py::object PyVarODE::py_kicks() const{
    return ORBIDYN_VISIT_ODE_VARIANT(
        const ode::chaos::VariationalODE<T, 0>& vode = varode<T>();
        View<T> res(vode.kick_values().data(), vode.kick_values().size());
        return py::cast(res);
    )
}

py::object PyVarODE::copy() const{
    return py::cast(PyVarODE(*this));
}


//===========================================================================================
//                                EXPLICIT TEMPLATE INSTANTIATIONS
//===========================================================================================




} // namespace ode::python