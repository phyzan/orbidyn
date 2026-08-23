#include <orbidyn/core/lib_impl/Tools_impl.hpp>


namespace ode::python {



PyFuncWrapper::PyFuncWrapper(const py::capsule& obj, py::ssize_t py_nsys, const py::array_t<py::ssize_t>& py_shape_out, py::ssize_t py_nargs, const std::string& scalar_type) :
    DtypeDispatcher(scalar_type),
    shape_out(size_t(py_shape_out.size())),
    n_sys(size_t(py_nsys)),
    n_args(size_t(py_nargs)){

        // set rhs_func
        dispatch_scalar_type<void>(this->scalar_type, [&]<typename T>(){
            this->rhs_func = open_capsule<raw_pyrhs_t<T>>(obj);
        });

        // set output shape
        auto output_shape_c = py::array_t<py::ssize_t, py::array::c_style | py::array::forcecast>(py_shape_out);
        ndspan::copy_array(this->shape_out.data(), output_shape_c.data(), this->shape_out.size());
        long s = 1;
        for (long i : this->shape_out){
            s *= i;
        }
        this->n_out = size_t(s);
    }


py::object PyFuncWrapper::call(const py::object& t, const py::iterable& py_q, const py::args& py_args) const{

    return std::visit([&]<typename T>(const raw_pyrhs_t<T>& rhs){
        // TODO : minimize memory allocations
        std::vector<T> q = to_vector<T>(py_q);
        if (size_t(q.size()) != n_sys || py_args.size() != n_args){
            throw py::value_error("Invalid array sizes in ode function call");
        }
        std::vector<T> args = to_vector<T>(py_args);
        Array<T> res(nullptr, shape_out.data(), shape_out.size());
        rhs(res.data(), py::cast<T>(t), q.data(), args.data());
        return py::cast(res);
    }, this->rhs_func);
}

//===========================================================================================
//                                      DtypeDispatcher
//===========================================================================================


DtypeDispatcher::DtypeDispatcher(const std::string& dtype_){
    this->scalar_type = getScalarType(dtype_);
}

DtypeDispatcher::DtypeDispatcher(ScalarType dtype_) : scalar_type(dtype_) {}


//===========================================================================================
//                                      Helper Functions
//===========================================================================================

template<>
pyshape_t getShape(const py::ssize_t& dim1, const pyshape_t& shape){
    pyshape_t result;
    result.reserve(1 + shape.size()); // Pre-allocate memory for efficiency
    result.push_back(dim1);        // Add the first element
    result.insert(result.end(), shape.begin(), shape.end()); // Append the original vector
    return result;
}

pyshape_t shape_of(const py::object& obj) {
    py::array arr = py::array::ensure(obj);
    const ssize_t* shape_ptr = arr.shape();  // Pointer to shape data
    auto ndim = static_cast<size_t>(arr.ndim());  // Number of dimensions
    pyshape_t res(shape_ptr, shape_ptr + ndim);
    return res;
}


std::vector<EventOptions> to_Options(const py::iterable& d) {
    std::vector<EventOptions> result;

    for (const py::handle& item : d) {
        auto opt = py::cast<EventOptions>(item);
        result.emplace_back(opt);
    }
    result.shrink_to_fit();
    return result;
}




} // namespace ode::python