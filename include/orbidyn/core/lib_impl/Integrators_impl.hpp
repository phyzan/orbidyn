#ifndef ORBIDYN_INTEGRATORS_IMPL_HPP
#define ORBIDYN_INTEGRATORS_IMPL_HPP


#include <odecraft/Core/Events_impl.hpp>
#include <odecraft/Core/BaseSolver_impl.hpp>
#include <odecraft/Core/RichBase_impl.hpp>
#include <odecraft/Core/ObjectiveSolver_impl.hpp>
#include <odecraft/OdeResult/OdeResult_impl.hpp>
#include <odecraft/Integrators/Solvers_impl.hpp>
#include <odecraft/Interpolation/Univariate/StateInterp_impl.hpp>
#include "../lib/Integrators.hpp"
#include "../lib/EventHandling.hpp"
#include "../pycast/pycast.hpp" // IWYU pragma: keep

namespace ode::python{


template<typename T, bool force_jac, typename Callable>
bool init_ode_data(Callable&& action, const py::object& py_rhs, const py::object& py_jac, const pyshape_t& state_shape, const py::iterable& py_args, const py::iterable& py_events){
    // passes a copy of the final form of data to the OdeData object. As a result, the referenced data object should not be modified for the entire lifetime of the solver constructed with its copy.
    constexpr ScalarType scalar_type = getScalarType<T>();
    std::string scalar_type_str = getScalarTypeName<T>();
    py::tuple tuple_args = py::tuple(py_args);
    size_t nsys = prod(state_shape);
    
    bool f_is_compiled = py::isinstance<PyFuncWrapper>(py_rhs) || py::isinstance<py::capsule>(py_rhs);
    bool jac_is_compiled = !py_jac.is_none() && (py::isinstance<PyFuncWrapper>(py_jac) || py::isinstance<py::capsule>(py_jac));
    std::vector<T> args = (f_is_compiled || jac_is_compiled) ? to_vector<T>(py_args) : std::vector<T>{};

    raw_pyrhs_t<T> rhs{nullptr};
    raw_pyrhs_t<T> jac{nullptr};

    bool use_pythonic_rhs = false;
    if (f_is_compiled){
        if (py::isinstance<PyFuncWrapper>(py_rhs)){
            //safe approach
            auto& func_wrapper = py_rhs.cast<PyFuncWrapper&>();
            if (func_wrapper.scalar_type != scalar_type){
                throw py::value_error("The scalar type of the provided ode function (" + std::string(getScalarTypeName(func_wrapper.scalar_type)) + ") does not match the scalar type of the solver (" + scalar_type_str + ")");
            } else if (func_wrapper.n_sys != nsys){
                throw py::value_error("The array size of the initial conditions differs from the ode system size");
            } else if (func_wrapper.n_args != args.size()){
                throw py::value_error("The number of the provided extra args (" + std::to_string(args.size()) + ") differs from the number of args specified for this ode system ("+std::to_string(func_wrapper.n_args)+").");
            }
            assert((std::holds_alternative<raw_pyrhs_t<T>>(func_wrapper.rhs_func)) && "The template parameter of init_ode_data `T` is different from the scalar type of the PyFuncWrapper of the rhs");
            rhs = std::get<raw_pyrhs_t<T>>(func_wrapper.rhs_func);
        } else {
            rhs = open_capsule<raw_pyrhs_t<T>>(py_rhs.cast<py::capsule>());
        }
    } else {
        use_pythonic_rhs = true;
    }

    bool use_pythonic_jac = false;
    bool jac_analytic = true;
    if (jac_is_compiled){
        if (py::isinstance<PyFuncWrapper>(py_jac)){
            //safe approach
            auto& jac_wrapper = py_jac.cast<PyFuncWrapper&>();
            if (jac_wrapper.scalar_type != scalar_type){
                throw py::value_error("The scalar type of the provided jacobian (" + std::string(getScalarTypeName(jac_wrapper.scalar_type)) + ") does not match the scalar type of the solver (" + scalar_type_str + ")");
            } else if (jac_wrapper.n_sys != nsys){
                throw py::value_error("The array size of the initial conditions differs from the ode system size that applied in the provided jacobian");
            } else if (jac_wrapper.n_args != args.size()){
                throw py::value_error("The array size of the given extra args differs from the number of args specified for the provided jacobian");
            }
            assert((std::holds_alternative<raw_pyrhs_t<T>>(jac_wrapper.rhs_func)) && "The template parameter of init_ode_data `T` is different from the scalar type of the PyFuncWrapper of the jacobian");
            jac = std::get<raw_pyrhs_t<T>>(jac_wrapper.rhs_func);
        } else {
            jac = open_capsule<raw_pyrhs_t<T>>(py_jac.cast<py::capsule>());
        }
    } else if (!py_jac.is_none()){
        use_pythonic_jac = true;
    } else {
        jac_analytic = false;
    }
    
    if (force_jac && !jac_analytic){
        throw py::value_error("No jacobian was provided, but the solver requires one");
    }

    // ======================== validate events ==========================
    for (py::handle ev : py_events){
        if (!py::isinstance<PyEvent>(ev)) {
            throw py::value_error("All objects in 'events' iterable argument must be instances of the Event class. Instance of type '" + std::string(py::str(py::type::of(ev))) + "' was found.");
        }
        const auto& _ev = ev.cast<const PyEvent&>();
        if (_ev.scalar_type != scalar_type){
            throw py::value_error("All event objects in 'events' must have scalar type " + scalar_type_str + ".");
        }
        _ev.check_sizes(nsys, args.size());
    }
    // ===================================================================

    // ===================== define functions for OdeData =======================
    auto rhs_lambda_cmp = [=](T* out, const T& t, const T* q){
        rhs(out, t, q, args.data());
    };

    auto rhs_lambda_py = [=](T* out, const T& t, const T* q){
        rhs_pythonic(out, t, q, state_shape, tuple_args, py_rhs);
    };

    auto jac_lambda_cmp = [=](T* out, const T& t, const T* q){
        jac(out, t, q, args.data());
    };

    auto jac_lambda_py = [=](T* out, const T& t, const T* q){
        jac_pythonic(out, t, q, state_shape, tuple_args, py_jac);
    };
    // ==========================================================================
    EventList<T> events = to_Events<T>(py_events, state_shape, tuple_args);

    auto explicit_jac_path = [&](){
        // pass a jacobian whose type is not nullptr at compile time
        if (use_pythonic_rhs && use_pythonic_jac){
            action(
                OdeData{
                    .Rhs = rhs_lambda_py,
                    .Jac = jac_lambda_py
                },
                std::move(events)
            );
        } else if (use_pythonic_rhs && !use_pythonic_jac){
            action(
                OdeData{
                    .Rhs = rhs_lambda_py,
                    .Jac = jac_lambda_cmp
                },
                std::move(events)
            );
        } else if (!use_pythonic_rhs && use_pythonic_jac){
            action(
                OdeData{
                    .Rhs = rhs_lambda_cmp,
                    .Jac = jac_lambda_py
                },
                std::move(events)
            );
        } else {
            action(
                OdeData{
                    .Rhs = rhs_lambda_cmp,
                    .Jac = jac_lambda_cmp
                },
                std::move(events)
            );
        }
    };

    if constexpr (force_jac){
        explicit_jac_path();
    } else if (jac != nullptr){
        explicit_jac_path();
    } else if (use_pythonic_rhs){
        action(
            OdeData{
                .Rhs = rhs_lambda_py
                // .Jac = nullptr implicitly
            },
            std::move(events)
        );
    } else {
        action(
            OdeData{
                .Rhs = rhs_lambda_cmp
                // .Jac =  nullptr implicitly
            },
            std::move(events)
        );
    }

    bool is_lowlevel = f_is_compiled && (jac_is_compiled || py_jac.is_none()) && all_are_lowlevel(py_events);
    return is_lowlevel;
}

template<typename Callable>
void py_advance_all_general(py::object& list, Callable&& func, int threads, bool display_progress){
    // Separate lists for each numeric type
    std::vector<integrator_t*> array;

    // Iterate through the list and identify each PySolver type
    for (const py::handle& item : list) {
        try {
            auto& pysolver = item.cast<PySolver&>();


            if (!pysolver.is_lowlevel) {
                throw py::value_error("All solvers in advance_all must use only compiled functions, and no pure python functions");
            }
            array.push_back(&pysolver.integrator);
        } catch (const py::cast_error&) {
            // If cast failed, throw an error
            throw py::value_error("List item is not a recognized PySolver object type.");
        }
    }

    const int num = (threads <= 0) ? omp_get_max_threads() : threads;
    int tot = 0;
    const int target = int(array.size());
    Clock clock;
    clock.start();

    std::exception_ptr thread_exception = nullptr;
    std::atomic<bool> error_flag{false};

    #pragma omp parallel for schedule(dynamic) num_threads(num)
    for (size_t i=0; i<array.size(); i++){
        if (error_flag.load()){ continue;}
        try {
            std::visit([&]<typename T>(pbox::owner<OdeRichSolver<T>>& solver) NDSPAN_LAMBDA_INLINE {
                func(*solver);
            }, *array[i]);
        } catch (...) {
            #pragma omp critical
            {
                if (!error_flag.load()) {
                    thread_exception = std::current_exception();
                    error_flag.store(true);
                }
            }
            continue;
        }

        #pragma omp critical
        {
            if (display_progress){
                show_progress(++tot, target, clock);
            }
        }
    }

    if (thread_exception) {
        std::rethrow_exception(thread_exception);
    }
    std::cout << std::endl << "Parallel integration completed in: " << clock.message() << std::endl;

}

} // namespace ode::python

#endif // ORBIDYN_INTEGRATORS_IMPL_HPP