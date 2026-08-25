#include <orbidyn/core/lib/History.hpp>
#include <orbidyn/core/lib/SubIntegrators.hpp>
#include <orbidyn/core/lib_impl/LowLevelOde_impl.hpp>
#include <orbidyn/core/lib_impl/Integrators_impl.hpp>
#include <orbidyn/core/lib_impl/Tools_impl.hpp>

namespace ode::python {

//===========================================================================================
//                                      PyODE
//===========================================================================================


PyODE::PyODE(const py::object& f, const py::object& t0, const py::iterable& py_q0, const py::object& jacobian, const py::object& rtol, const py::object& atol, const py::object& min_step, const py::object& max_step, const py::object& stepsize, int dir, const py::iterable& py_args, const py::iterable& py_events, const py::str& method, const std::string& scalar_type) : DtypeDispatcher(scalar_type){
    this->init_scalar_type();
    this->state_dims = shape_of(py_q0);
    ORBIDYN_MODIFY_ODE_VARIANT(
        this->is_lowlevel = init_ode_data<T, false>([&](auto ode_obj, EventList<T>&& events){
            std::vector<T> q0 = to_vector<T>(py_q0);
            ode_ptr = pbox::make_box<ODE<T>>(std::move(ode_obj), py::cast<T>(t0), View1D{q0.data(), q0.size()}, py::cast<T>(rtol), py::cast<T>(atol), py::cast<T>(min_step), (max_step.is_none() ? 0 : max_step.cast<T>()), py::cast<T>(stepsize), dir, std::move(events), getIntegrator(method));
        }, f, jacobian, this->state_dims, py_args, py_events);
    )
}

PyODE::PyODE(ode_t ode_owner, pyshape_t state_shape, bool ode_is_lowlevel) : DtypeDispatcher(visit_scalar_type(ode_owner)), ode(std::move(ode_owner)), state_dims(std::move(state_shape)), is_lowlevel(ode_is_lowlevel){}

PyODE::PyODE(const std::string& scalar_type) : DtypeDispatcher(scalar_type){
    this->init_scalar_type();
}

void PyODE::init_scalar_type(){
    // set the integrator the ode variant type
    // even though it is uninitialized
    dispatch_scalar_type<void>(this->scalar_type,
        [&]<typename T>(){
            this->ode = pbox::owner<ODE<T>>{};
        }
    );
}


py::object PyODE::call_Rhs(const py::object& t, const py::iterable& py_q) const{
    return ORBIDYN_VISIT_ODE_VARIANT(
        std::vector<T> q = to_vector<T>(py_q);
        if (size_t(q.size()) != ode_ptr->nsys()){
            throw py::value_error("Invalid size of state array in call to rhs");
        }
        Array1D<T> qdot(ode_ptr->nsys());
        ode_ptr->Rhs(qdot.data(), py::cast<T>(t), q.data());
        return py::cast(qdot);
    )
}

py::object PyODE::call_Jac(const py::object& t, const py::iterable& py_q) const{
    return ORBIDYN_VISIT_ODE_VARIANT(
        std::vector<T> q = to_vector<T>(py_q);
        if (size_t(q.size()) != ode_ptr->nsys()){
            throw py::value_error("Invalid size of state array in call to rhs");
        }
        Array2D<T, 0, 0, ndspan::Allocation::Heap, ndspan::Layout::F> jac(ode_ptr->nsys(), ode_ptr->nsys());
        ode_ptr->Jac(jac.data(), py::cast<T>(t), q.data(), nullptr);
        for (size_t i=0; i<ode_ptr->nsys(); i++){
            for (size_t j=i+1; j<ode_ptr->nsys(); j++){
                T tmp = jac(i, j);
                jac(i, j) = jac(j, i);
                jac(j, i) = tmp;
            }
        }
        return py::cast(View<T, ndspan::Layout::C, 0, 0>(jac.data(), ode_ptr->nsys(), ode_ptr->nsys()));
    )
}

py::object PyODE::py_integrate(const py::object& interval, const py::object& t_eval, const py::iterable& event_options, int max_prints){
    return ORBIDYN_MODIFY_ODE_VARIANT(
        pbox::Box<OdeResult<T>> result = pbox::make_box<OdeResult<T>>();
        if (t_eval.is_none()){
            ode_ptr->integrate(result.get_raw_pointer(), py::cast<T>(interval), to_Options(event_options), nullptr, max_prints);
        }else{
            std::vector<T> t_seq = to_vector<T>(t_eval.cast<py::iterable>());
            ode_ptr->integrate(result.get_raw_pointer(), py::cast<T>(interval), t_seq, to_Options(event_options), nullptr, max_prints);
        }
        return py::cast(PyOdeResult(std::move(result), this->state_dims));
    )
}

py::object PyODE::py_rich_integrate(const py::object& interval, const py::iterable& event_options, int max_prints){
    return ORBIDYN_MODIFY_ODE_VARIANT(
        pbox::Box<OdeSolution<T>> result = pbox::make_box<OdeSolution<T>>();
        ode_ptr->rich_integrate(*result.get_raw_pointer(), py::cast<T>(interval), to_Options(event_options), nullptr, max_prints);
        return py::cast(PyOdeSolution(std::move(result), this->state_dims));
    )
}

py::object PyODE::py_integrate_until(const py::object& t, const py::object& t_eval, const py::iterable& event_options, int max_prints){

    return ORBIDYN_MODIFY_ODE_VARIANT(
        pbox::Box<OdeResult<T>> result = pbox::make_box<OdeResult<T>>();
        if (t_eval.is_none()){
            ode_ptr->integrate_until(result.get_raw_pointer(), py::cast<T>(t), to_Options(event_options), nullptr, max_prints);
        } else {
            std::vector<T> t_seq = to_vector<T>(t_eval.cast<py::iterable>());
            ode_ptr->integrate_until(result.get_raw_pointer(), py::cast<T>(t), t_seq, to_Options(event_options), nullptr, max_prints);
        }
        return py::cast(PyOdeResult(std::move(result), this->state_dims));
    )
}

py::object PyODE::t_array() const{
    return ORBIDYN_VISIT_ODE_VARIANT(
        return py::cast(View<T>(ode_ptr->t().data(), ode_ptr->t().size()));    
    )
}

py::object PyODE::q_array() const{

    return ORBIDYN_VISIT_ODE_VARIANT(
        auto shape = getShape<size_t>(py::ssize_t(ode_ptr->t().size()), this->state_dims);
        return py::cast(View<T>(ode_ptr->q().data(), shape.data(), shape.size()));
    )
}

py::tuple PyODE::event_data(const py::str& event) const{
    return ORBIDYN_VISIT_ODE_VARIANT(
        const OrbitData<T>& event_data = ode_ptr->event_data(event); //check if event exists
        auto shape = getShape<size_t>(py::ssize_t(event_data.size()), this->state_dims);
        View1D<T> t_view = event_data.t_view();
        View2D<T, 0, 0> q_view = event_data.q_view();
        View<T> true_view(q_view.data(), shape.data(), shape.size());
        return py::make_tuple(py::cast(t_view), py::cast(true_view));
    )
}

py::object PyODE::copy() const{
    return py::cast(PyODE(*this));
}

py::object PyODE::solver_copy() const{
    return ORBIDYN_VISIT_ODE_VARIANT(

        std::unique_ptr<OdeSolver<T>> solver_clone = ode_ptr->solver()->clone();

        pbox::Box<OdeRichSolver<T>> rich_clone = std::unique_ptr<OdeRichSolver<T>>(static_cast<OdeRichSolver<T>*>(solver_clone.release()));

        switch (ode_ptr->solver()->get_method()){
            case Integrator::RK45:
                return py::cast(PyRK45(std::move(rich_clone), this->is_lowlevel));
            case Integrator::DOP853:
                return py::cast(PyDOP853(std::move(rich_clone), this->is_lowlevel));
            case Integrator::RK23:
                return py::cast(PyRK23(std::move(rich_clone), this->is_lowlevel));
            case Integrator::BDF:
                return py::cast(PyBDF(std::move(rich_clone), this->is_lowlevel));
            case Integrator::RK4:
                return py::cast(PyRK4(std::move(rich_clone), this->is_lowlevel));
            default:
                throw py::value_error("Unregistered solver!");
        }
    )
}


py::object PyODE::Nsys() const{
    return ORBIDYN_VISIT_ODE_VARIANT(
        return py::cast(ode_ptr->nsys());
    )
}

py::object PyODE::runtime() const{
    return ORBIDYN_VISIT_ODE_VARIANT(
        return py::cast(ode_ptr->runtime());
    )
}

py::object PyODE::diverges() const{
    return ORBIDYN_VISIT_ODE_VARIANT(
        return py::cast(ode_ptr->diverges());
    )
}

py::object PyODE::is_dead() const{
    return ORBIDYN_VISIT_ODE_VARIANT(
        return py::cast(ode_ptr->is_dead());
    )
}

void PyODE::reset() {
    ORBIDYN_MODIFY_ODE_VARIANT(
        ode_ptr->reset();
    )
}

void PyODE::clear() {
    return ORBIDYN_MODIFY_ODE_VARIANT(
        return ode_ptr->clear();
    )
}


//===========================================================================================
//                                      Additional functions
//===========================================================================================

template<typename... T>
using variant_vec = std::variant<std::vector<T>...>;

using vec_t = variant_vec<ORBIDYN_SCALARS>;

void py_integrate_all(py::object& list, double interval, const py::object& t_eval, const py::iterable& event_options, int threads, bool display_progress){
    // Separate lists for each numeric type
    std::unordered_map<ScalarType, vec_t> step_seq;
    
    auto options = to_Options(event_options);

    // Iterate through the list and identify each PyODE type
    std::vector<ode_t*> array;
    array.reserve(py::len(list));
    for (const py::handle& item : list) {
        try {
            auto& pyode = item.cast<PyODE&>();
            if (!pyode.is_lowlevel) {
                throw py::value_error("All ODE's in integrate_all must use only compiled functions, and no pure python functions");
            } else {
                array.push_back(&pyode.ode);
                if ((!step_seq.contains(pyode.scalar_type)) && !t_eval.is_none()){
                    dispatch_scalar_type<void>(pyode.scalar_type, [&]<typename T>(){
                        step_seq[pyode.scalar_type] = to_vector<T>(t_eval.cast<py::iterable>());
                    });

                }
            }
        } catch (const py::cast_error&) {
            // If cast failed, throw an error
            throw py::value_error("List item is not a recognized LowLevelODE object type.");
        }
    }


    const int num = (threads <= 0) ? omp_get_max_threads() : threads;
    int tot = 0;
    const int target = int(array.size());
    Clock clock;
    clock.start();   
    

    const bool t_eval_is_none = t_eval.is_none();
    #pragma omp parallel for schedule(dynamic) num_threads(num)
    for (size_t i=0; i<array.size(); i++){

        ScalarType scalar_type = visit_scalar_type(*array[i]);
        dispatch_scalar_type<void>(
            scalar_type,
            [&]<typename T>(){
                owner<ODE<T>>& ode_ref = std::get<pbox::owner<ODE<T>>>(*array[i]);
                if (t_eval_is_none){
                    ode_ref->integrate(nullptr, T(interval), options);
                } else {
                    const std::vector<T>& steps = std::get<std::vector<T>>(step_seq[scalar_type]);
                    ode_ref->integrate(nullptr, T(interval), steps, options);
                }
            }
        );

        #pragma omp critical
        {
            if (display_progress){
                show_progress(++tot, target, clock);
            }
        }
    }

    std::cout << std::endl << "Parallel integration completed in: " << clock.message() << std::endl;
}


} // namespace ode::python