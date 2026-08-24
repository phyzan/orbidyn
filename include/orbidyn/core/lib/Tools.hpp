#ifndef ORBIDYN_TOOLS_HPP
#define ORBIDYN_TOOLS_HPP



#include <odecraft/Core/Events.hpp>
#include "../Dispatcher.hpp"

namespace ode::python {


namespace py = pybind11;

using pyshape_t = std::vector<py::ssize_t>;

template<typename T>
using raw_pyrhs_t = void(*)(T*, const T&, const T*, const T*); // f(t, q, args) -> array

template<typename T>
using raw_pyobjfun_t = T(*)(const T&, const T*, const T*); // f(t, q, args) -> T

template<typename... T>
using variant_raw_rhs_t = std::variant<raw_pyrhs_t<T>...>;

template<typename... T>
using variant_raw_pyobjfun_t = std::variant<raw_pyobjfun_t<T>...>;

using RawPyRhs_t    = variant_raw_rhs_t<ORBIDYN_SCALARS>; // (T* out, const T& t, const T* q, const T* args) -> void
using RawPyObjFun_t = variant_raw_pyobjfun_t<ORBIDYN_SCALARS>; // (const T& t, const T* q, const T* args) -> T



// Generic over any std::variant<pbox::owner<Class<T>>...> (solvers, odes, results alike),
// so it lives here rather than next to any one of them.
template<template<typename> typename Class, typename... T>
inline ScalarType visit_scalar_type(const std::variant<pbox::owner<Class<T>>...>& obj){
    return std::visit([]<typename A>(const pbox::owner<Class<A>>&){
        return getScalarType<A>();
    }, obj);
}


struct DtypeDispatcher{

    DtypeDispatcher(const std::string& dtype_);

    DtypeDispatcher(ScalarType dtype_);

    ScalarType scalar_type;
};


struct PyFuncWrapper : DtypeDispatcher {

    RawPyRhs_t rhs_func;
    pyshape_t shape_out;
    size_t n_sys;
    size_t n_args;
    size_t n_out;

    PyFuncWrapper(const py::capsule& obj, py::ssize_t Nsys, const py::array_t<py::ssize_t>& output_shape, py::ssize_t Nargs, const std::string& scalar_type);

    py::object call(const py::object& t, const py::iterable& py_q, const py::args& py_args) const;
};



template<typename T>
std::vector<T> to_vector(const py::iterable &obj);

std::vector<EventOptions> to_Options(const py::iterable& d);

template<typename Int>
std::vector<Int> getShape(const py::ssize_t& dim1, const pyshape_t& shape);

template<typename Scalar, typename ArrayType>
py::array_t<Scalar> to_numpy(const ArrayType& array, const pyshape_t& state_shape = {});

template<typename T>
py::array_t<T> array(T* data, const pyshape_t& shape);

pyshape_t shape_of(const py::object& obj);

template<typename T>
NDSPAN_INLINE void pass_values(T* out, const py::iterable& obj, size_t n){
    if (py::len(obj) != n){
        throw py::value_error("Expected iterable of length " + std::to_string(n));
    }
    for (py::handle item : obj){
        *(out++) = py::cast<T>(item);
    }
}

template<typename T>
inline T open_capsule(const py::capsule& f){
    void* ptr = f.get_pointer();
    if (ptr == nullptr){
        return nullptr;
    } else{
        return reinterpret_cast<T>(ptr);
    }
}

template<typename T>
inline void arrcpy(T* dst, const T* src, size_t size){
    for (size_t i=0; i<size; i++){
        dst[i] = src[i];
    }
}


// ================================ RHS-like functions ========================================

template<typename T>
inline void rhs_pythonic(T* out, const T& t, const T* q, const pyshape_t& shape, const py::tuple& py_args, const py::function& rhs_func) {
    py::array_t<T> pyres = rhs_func(t, py::array_t<T>(shape, q), *py_args);
    auto r = pyres.template unchecked<1>();
    for (py::ssize_t i = 0; i < r.shape(0); i++) {
        out[i] = r(i);
    }
}

template<typename T>
inline void jac_pythonic(T* out, const T& t, const T* q, const pyshape_t& shape, const py::tuple& py_args, const py::function& rhs_func) {
    py::array_t<T> pyres = rhs_func(t, py::array_t<T>(shape, q), *py_args);
    auto r = pyres.template unchecked<2>();
    py::ssize_t n = r.shape(0);
    ndspan::MutView<T, ndspan::Layout::F, 0, 0> jm{out, n, n}; // Column-major layout
    for (py::ssize_t j = 0; j < n; j++){
        for (py::ssize_t i = 0; i < n; i++) {
            jm(i, j) = r(i, j);
        }
    }
}

template<typename T>
inline T objfun_pythonic(const T& t, const T* q, const pyshape_t& shape, const py::tuple& py_args, const py::function& py_objfun) {
    py::array_t<T> vector{shape, q};
    return py_objfun(t, vector, *py_args).template cast<T>();
}


template<>
inline void rhs_pythonic<mpreal_t>(mpreal_t* out, const mpreal_t& t, const mpreal_t* q, const pyshape_t& shape, const py::tuple& py_args, const py::function& rhs_func) {
    py::iterable pyres = rhs_func(t, View<mpreal_t>(q, shape.data(), shape.size()), *py_args);
    std::vector<mpreal_t> arr = to_vector<mpreal_t>(pyres);
    arrcpy(out, arr.data(), arr.size());
}


template<>
inline void jac_pythonic<mpreal_t>(mpreal_t* out, const mpreal_t& t, const mpreal_t* q, const pyshape_t& shape, const py::tuple& py_args, const py::function& rhs_func) {
    py::array pyres = rhs_func(t, View<mpreal_t>(q, shape.data(), shape.size()), *py_args);
    auto r = pyres.template unchecked<mpreal_t, 2>();
    py::ssize_t n = r.shape(0);
    ndspan::MutView<mpreal_t, ndspan::Layout::F, 0, 0> jm{out, n, n}; // Column-major layout
    for (py::ssize_t j = 0; j < n; j++){
        for (py::ssize_t i = 0; i < n; i++) {
            jm(i, j) = r(i, j);
        }
    }
}

template<>
inline mpreal_t objfun_pythonic<mpreal_t>(const mpreal_t& t, const mpreal_t* q, const pyshape_t& shape, const py::tuple& py_args, const py::function& py_objfun) {
    // py::array_t<T> vector{shape, q};
    View<mpreal_t> vector{q, shape.data(), shape.size()};
    return py_objfun(t, vector, *py_args).template cast<mpreal_t>();
}

} // namespace ode::python

#endif // ORBIDYN_TOOLS_HPP