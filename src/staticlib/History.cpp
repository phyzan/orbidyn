#include <odecraft/Interpolation/Univariate/StateInterp_impl.hpp>
#include <odecraft/OdeResult/OdeResult_impl.hpp>
#include <orbidyn/core/lib/History.hpp>
#include <orbidyn/core/lib_impl/Tools_impl.hpp>


#define ORBIDYN_VISIT_ODERESULT_VARIANT(...) \
    std::visit([&]<typename T>(const pbox::owner<OdeResult<T>>& hist){ \
        __VA_ARGS__ \
    }, this->history); 

namespace ode::python {

//===========================================================================================
//                                      PyOdeResult
//===========================================================================================



PyOdeResult::PyOdeResult(result_t result, const pyshape_t& state_shape): DtypeDispatcher(visit_scalar_type(result)), history(std::move(result)), state_dims(state_shape){}

py::object PyOdeResult::t() const{
    return ORBIDYN_VISIT_ODERESULT_VARIANT(
        return py::cast(View<T>(hist->t().data(), hist->t().size()));
    )
}

py::object PyOdeResult::q() const{
    return ORBIDYN_VISIT_ODERESULT_VARIANT(
        auto shape = getShape<size_t>(py::ssize_t(hist->t().size()), this->state_dims);
        return py::cast(View<T>(hist->q().data(), shape.data(), shape.size()));
    )
}

py::tuple PyOdeResult::event_data(const py::str& event) const{
    return ORBIDYN_VISIT_ODERESULT_VARIANT(
        const OrbitData<T>& event_data = hist->event_data().data(event); //check if event exists
        auto shape = getShape<size_t>(py::ssize_t(event_data.size()), this->state_dims);
        View1D<T> t_view = event_data.t_view();
        View2D<T, 0, 0> q_view = event_data.q_view();
        View<T> true_view(q_view.data(), shape.data(), shape.size());
        return py::make_tuple(py::cast(t_view), py::cast(true_view));
    )
}


py::bool_ PyOdeResult::diverges() const{
    return ORBIDYN_VISIT_ODERESULT_VARIANT(
        return py::cast(hist->diverges());
    )
}

py::bool_ PyOdeResult::success() const{
    return ORBIDYN_VISIT_ODERESULT_VARIANT(
        return py::cast(hist->success());
    )
}

py::float_ PyOdeResult::runtime() const{
    return ORBIDYN_VISIT_ODERESULT_VARIANT(
        return py::cast(hist->runtime());
    )
}

py::str PyOdeResult::message() const{
    return ORBIDYN_VISIT_ODERESULT_VARIANT(
        return py::cast(hist->message());
    )
}

void PyOdeResult::examine() const{
    ORBIDYN_VISIT_ODERESULT_VARIANT(
        return hist->examine();
    )
}



//===========================================================================================
//                                      PyOdeSolution
//===========================================================================================


PyOdeSolution::PyOdeSolution(result_t result, const pyshape_t& state_shape) : PyOdeResult(std::move(result), state_shape), nsys(prod(state_shape)) {}


py::object PyOdeSolution::get_frame(const py::object& t) const{
    return ORBIDYN_VISIT_ODERESULT_VARIANT(
        return py::cast(Array<T>(hist.template cast<OdeSolution<T>>()->operator()(t.cast<T>()).data(), this->state_dims.data(), this->state_dims.size()));
    )
}


py::object PyOdeSolution::get_array(const py::array& py_array) const{
    const auto nt = size_t(py_array.size());
    pyshape_t final_shape(py_array.shape(), py_array.shape()+py_array.ndim());
    final_shape.insert(final_shape.end(), this->state_dims.begin(), this->state_dims.end());
    return ORBIDYN_VISIT_ODERESULT_VARIANT(
        Array<T> res(nullptr, final_shape.data(), final_shape.size());
        const auto* solution = hist.template cast<OdeSolution<T>>();

        // Extract array values and cast them to T using Python's item access
        for (size_t i=0; i<nt; i++){
            py::object item = py_array.attr("flat")[py::int_(i)];
            T t_value = py::cast<T>(item);
            ndspan::copy_array(res.data()+i*nsys, solution->operator()(t_value).data(), nsys);
        }
        return py::cast(res);
    )
}


py::object PyOdeSolution::operator()(const py::object& t) const{
    try {
        // Try to convert t to a numpy array
        py::array arr = py::array::ensure(t);
        return this->get_array(arr);
    } catch (const py::cast_error&) {
        // If conversion fails, treat as a scalar
        return this->get_frame(t);
    }
}

//===========================================================================================
//                                EXPLICIT TEMPLATE INSTANTIATIONS
//===========================================================================================



} // namespace ode::python