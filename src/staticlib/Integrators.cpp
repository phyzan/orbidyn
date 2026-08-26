#include <odecraft/Core/SolverFactory.hpp>
#include <odecraft/Core/BaseSolver_impl.hpp>
#include <orbidyn/core/lib_impl/Integrators_impl.hpp>
#include <orbidyn/core/lib_impl/Tools_impl.hpp>


namespace ode::python {

//===========================================================================================
//                                      PyConstSolver
//===========================================================================================

PyConstSolver::PyConstSolver(const py::object& f, const py::object& t0, const py::iterable& py_q0, const py::object& jac, const py::object& rtol, const py::object& atol, const py::object& min_step, const py::object& max_step, const py::object& stepsize, int dir, const py::iterable& py_args, const py::iterable& py_events, const std::string& scalar_type, const std::string& method) : DtypeDispatcher(scalar_type){
    this->init_scalar_type();
    dispatch_scalar_type<void>(this->scalar_type, [&]<typename T>(){
        this->is_lowlevel = init_ode_data<T, false>([&](auto ode_obj, EventList<T>&& events){
            std::vector<T> q0 = to_vector<T>(py_q0);
            this->integrator = make_solver<UtilPolicy::RichVirtual>(getIntegrator(method), std::move(ode_obj), py::cast<T>(t0), View1D{q0.data(), q0.size()}, py::cast<T>(rtol), py::cast<T>(atol), py::cast<T>(min_step), (max_step.is_none() ? 0 : max_step.cast<T>()), py::cast<T>(stepsize), dir, std::move(events));
        }, f, jac, shape_of(py_q0), py_args, py_events);
    });
}

PyConstSolver::PyConstSolver(integrator_t solver, bool is_lowlevel) : DtypeDispatcher(visit_scalar_type(solver)), integrator(std::move(solver)), is_lowlevel(is_lowlevel){}

PyConstSolver::PyConstSolver(const std::string& scalar_type) : DtypeDispatcher(scalar_type){
    this->init_scalar_type();
}

void PyConstSolver::init_scalar_type(){
    // set the integrator the corresponding variant type
    // even though it is uninitialized
    dispatch_scalar_type<void>(this->scalar_type,
        [&]<typename T>(){
            this->integrator = pbox::owner<OdeRichSolver<T>>{};
        }
    );
}


py::object PyConstSolver::t0() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(solver->get_ics().t());
    )
}

py::object PyConstSolver::q0() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        const T* q0_ptr = solver->get_ics().vector();
        return py::cast(View1D<T>(q0_ptr, solver->get_nsys()));
    )
}

int PyConstSolver::direction() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return solver->get_direction();
    )
}

py::object PyConstSolver::t() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(solver->get_time());
    )
}

py::object PyConstSolver::t_old() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(solver->get_old_time());
    )
}

py::object PyConstSolver::q() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(solver->get_vector());
    )
}

py::object PyConstSolver::q_old() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(solver->get_old_vector());
    )
}

py::object PyConstSolver::stepsize() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(solver->get_stepsize());
    )
}

py::object PyConstSolver::diverges() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(solver->get_diverges());
    )
}

py::object PyConstSolver::is_dead() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(solver->get_is_dead());
    )
}

py::object PyConstSolver::nsys() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(solver->get_nsys());
    )
}

py::object PyConstSolver::rhs_eval_count() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(solver->get_rhs_eval_count());
    )
}

py::object PyConstSolver::jac_eval_count() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(solver->get_jac_eval_count());
    )
}


void PyConstSolver::show_state(int digits) const{
    ORBIDYN_VISIT_SOLVER_VARIANT(
        solver->show_state(digits);
    )
}

py::object PyConstSolver::py_rhs(const py::object& t, const py::iterable& py_q) const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        size_t nsys = solver->get_nsys();
        Array1D<T> tmp(2*nsys);
        if (size_t(py::len(py_q)) != nsys){
            throw py::value_error("Invalid size of state array in call to rhs");
        }
        pass_values(tmp.data()+nsys, py_q, nsys);
        solver->get_rhs(tmp.data(), py::cast<T>(t), tmp.data()+nsys);
        return py::cast(View1D<T>(tmp.data(), nsys));
    )
}

py::object PyConstSolver::py_jac(const py::object& t, const py::iterable& py_q) const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        size_t nsys = solver->get_nsys();
        Array1D<T> q(nsys);
        Array2D<T> jac(nsys, nsys);
        if (size_t(py::len(py_q)) != nsys){
            throw py::value_error("Invalid size of state array in call to rhs");
        }
        pass_values(q.data(), py_q, nsys);
        solver->get_jac(jac.data(), t.cast<T>(), q.data(), nullptr);
        for (size_t i=0; i<nsys; i++){
            for (size_t j=i+1; j<nsys; j++){
                std::swap(jac(i, j), jac(j, i));
            }
        }
        return py::cast(jac);
    )
}

py::tuple PyConstSolver::timeit_rhs(const py::object& t, const py::iterable& py_q) const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        size_t nsys = solver->get_nsys();
        Array1D<T> tmp(2*nsys);
        if (size_t(py::len(py_q)) != nsys){
            throw py::value_error("Invalid size of state array in call to rhs");
        }
        pass_values(tmp.data()+nsys, py_q, nsys);
        auto start = std::chrono::high_resolution_clock::now();
        solver->get_rhs(tmp.data(), py::cast<T>(t), tmp.data()+nsys);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        return py::make_tuple(py::cast(duration.count()), py::cast(View1D<T>(tmp.data(), nsys)));
    )
}


py::tuple PyConstSolver::timeit_jac(const py::object& t, const py::iterable& py_q) const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        size_t nsys = solver->get_nsys();
        Array1D<T> q(nsys);
        Array2D<T> jac(nsys, nsys);
        if (size_t(py::len(py_q)) != nsys){
            throw py::value_error("Invalid size of state array in call to rhs");
        }
        pass_values(q.data(), py_q, nsys);
        auto start = NOW;
        solver->get_jac(jac.data(), t.cast<T>(), q.data(), nullptr);
        auto end = NOW;
        std::chrono::duration<double, std::milli> duration = end - start;
        for (size_t i=0; i<nsys; i++){
            for (size_t j=i+1; j<nsys; j++){
                std::swap(jac(i, j), jac(j, i));
            }
        }
        return py::make_tuple(py::cast(duration.count()), py::cast(jac));
    )
}


py::str PyConstSolver::status() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        return py::cast(solver->get_status());
    )
}

py::object PyConstSolver::copy() const{
    return py::cast(PySolver(this->integrator, this->is_lowlevel));
}

bool PyConstSolver::py_at_event(const py::object& event_name) const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        if (solver->get_event_col().size() == 0){
            throw py::value_error("This solver contains no events to check for");
        } else if (event_name.is_none()){
            return solver->get_at_event();
        } else {
            try {
                auto name = event_name.cast<py::str>().cast<std::string>();
                int event_idx = solver->get_event_idx(name);
                if (event_idx == -1){
                    throw py::value_error("No event with name '" + name + "' found in the solver");
                }
                return solver->get_at_event(event_idx);
            } catch (const py::cast_error&){
                throw py::value_error("Event parameter must be a string name of an event in the solver (or None to check for any event)");
            }
        }
    )
}

py::object PyConstSolver::current_event() const{
    return ORBIDYN_VISIT_SOLVER_VARIANT(
        if (solver->get_event_col().size() == 0){
            return py::object(py::none());
        } else if (EventState<T> ev = solver->get_current_event()){
            return py::cast(solver->get_event_col().event(ev.idx).name());
        } else {
            return py::object(py::none());
        }
    )
}

//===========================================================================================
//                                          PySolver
//===========================================================================================

PySolver::PySolver(const PyConstSolver& other) : PyConstSolver(other) {}

py::object PySolver::advance() {
    return ORBIDYN_MODIFY_SOLVER_VARIANT(
        return py::cast(solver->do_advance());
    )
}

py::tuple PySolver::timeit_step() {
    return ORBIDYN_MODIFY_SOLVER_VARIANT(
        auto start = NOW;
        bool success = solver->do_advance();
        auto end = NOW;
        std::chrono::duration<double, std::milli> duration = end - start;
        return py::make_tuple(py::cast(duration.count()), py::cast(success));
    )
}

py::object PySolver::advance_to_event(const py::object& events) {
    return ORBIDYN_MODIFY_SOLVER_VARIANT(
        if (solver->get_event_col().size() == 0){
            throw py::value_error("This solver contains no events to advance to");
        }else if (events.is_none()){
            return py::cast(solver->do_advance_to_event());
        }

        std::vector<size_t> event_list;
        if (py::isinstance<py::str>(events)){
            std::string name = events.cast<std::string>();
            if (int event_idx = solver->get_event_idx(name); event_idx != -1){
                event_list = {size_t(event_idx)};
                return py::cast(solver->do_advance_to_event(event_list));
            } else {
                throw py::value_error("No event with name '" + name + "' found in the solver");
            }
        } else if (py::isinstance<py::iterable>(events)){
            try{
                auto py_event_list = events.cast<py::iterable>();
                for (const auto& item : py_event_list){
                    std::string name = item.cast<std::string>();
                    if (int event_idx = solver->get_event_idx(name); event_idx != -1){
                        event_list.push_back(size_t(event_idx));
                    } else {
                        throw py::value_error("No event with name '" + name + "' found in the solver");
                    }
                }
                return py::cast(solver->do_advance_to_event(event_list));
            } catch (const py::cast_error&){
                throw py::value_error("Event parameter must be either a string name of an event in the solver, or an iterable of such strings");
            }
        } else {
            throw py::value_error("Event parameter must be either a string name of an event in the solver, or an iterable of such strings");
        }
    )
}

py::object PySolver::advance_until(const py::object& time, const py::object& observer, const py::object& extra_steps) {
    return ORBIDYN_MODIFY_SOLVER_VARIANT(

        std::vector<T> steps = {};
        if (!extra_steps.is_none()){
            //try to cast to iterable
            try{
                auto py_steps = extra_steps.cast<py::iterable>();
                steps = to_vector<T>(py_steps);
            } catch (const py::cast_error&){
                throw py::value_error("extra_steps parameter must be an iterable of time points to observe (or None)");
            }
        }
        if (observer.is_none() && steps.size() == 0){
            return py::cast(solver->do_advance_until(time.cast<T>()));
        }
        py::function py_obs;
        try{
            py_obs = observer.cast<py::function>();
        } catch (const py::cast_error&){
            throw py::value_error("The observer parameter must be a function that takes no arguments");
        }
        observer_t<T> obs = [py_obs, this](const T&, const T*, const T*) -> bool {
            py::object result = py_obs(this);
            if (result.is_none()) return true;
            return result.cast<bool>();
        };
        
        if (extra_steps.is_none()){
            return py::cast(solver->do_advance_until(time.cast<T>(), obs));
        }else{
            return py::cast(solver->do_advance_until(time.cast<T>(), obs, View1D<T>(steps.data(), steps.size())));
        }
    )
}

void PySolver::reset() {
    return ORBIDYN_MODIFY_SOLVER_VARIANT(
        return solver->do_reset();
    )
}

bool PySolver::set_ics(const py::object& t0, const py::iterable& py_q0, const py::object& dt, int direction) {
    if (direction != 1 && direction != -1 && direction != 0){
        throw py::value_error("Direction must be either +1 or -1 or 0 (default)");
    }
    return ORBIDYN_MODIFY_SOLVER_VARIANT(
        if (dt.cast<T>() < 0){
            throw py::value_error("Stepsize cannot be negative");
        }
        std::vector<T> q0 = to_vector<T>(py_q0);
        if (size_t(q0.size()) != solver->get_nsys()){
            throw py::value_error("Invalid size of initial condition array");
        }
        return solver->do_set_ics(t0.cast<T>(), q0.data(), dt.cast<T>(), direction);
    )
}

void PySolver::kill(std::string reason) { ORBIDYN_MODIFY_SOLVER_VARIANT( solver->do_kill(std::move(reason)); ) }

//===========================================================================================
//                                      Additional functions
//===========================================================================================


void py_advance_all(py::object& list, double t_goal, int threads, bool display_progress){
    py_advance_all_general(list, [&]<typename T>(OdeRichSolver<T>& solver){
        solver.do_advance_until(T(t_goal));
    }, threads, display_progress);

}

void py_advance_all_to_event(py::object& list, const py::object& events, double tmax, int threads, bool display_progress){
    std::vector<std::string> event_list;
    if (py::isinstance<py::str>(events)){
        event_list = {events.cast<std::string>()};
    } else if (py::isinstance<py::iterable>(events)){
        try{
            auto py_event_list = events.cast<py::iterable>();
            for (const auto& item : py_event_list){
                event_list.push_back(item.cast<std::string>());
            }
        } catch (const py::cast_error&){
            throw py::value_error("Event parameter must be either a string name of an event in the solver, or an iterable of such strings");
        }
    } else {
        throw py::value_error("Event parameter must be either a string name of an event in the solver, or an iterable of such strings");
    }
    py_advance_all_general(list, [&]<typename T>(OdeRichSolver<T>& solver){
        solver.do_advance_to_event(T(tmax), event_list);
    }, threads, display_progress);
}


} // namespace ode::python
