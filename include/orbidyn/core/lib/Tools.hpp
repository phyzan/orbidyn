#ifndef ORBIDYN_TOOLS_HPP
#define ORBIDYN_TOOLS_HPP



#include <algorithm>
#include <atomic>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>
#include "../Dispatcher.hpp"

namespace ode::python {


namespace py = pybind11;

using pyshape_t = std::vector<py::ssize_t>;

template<typename T>
using CArray = py::array_t<T, py::array::c_style | py::array::forcecast>;

using c_array_t = CArray<double>;

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


// Every extension module links its own copy of odecraft, and with it its own lazex scratch
// buffers for mpreal_t, which ode::crafted::set_mpreal_prec resizes only for the module it is
// called from. So each module exposes its own copy as _set_mpreal_prec, and
// orbidyn.set_mpreal_prec (__init__.py) calls all of them.
inline void bind_mpreal_prec(py::module_& m){
    m.def("_set_mpreal_prec", &::ode::crafted::set_mpreal_prec, py::arg("bits"));
}


// Runs action(0), ..., action(range-1) on `threads` threads (all hardware threads if <= 0).
//
// The threads are created afresh on every call rather than kept in a pool (as OpenMP does).
// A pooled thread keeps the scratch buffers that every module's code created on it, at the
// precision of the call that created them, and set_mpreal_prec can only resize those of the
// module it belongs to. A fresh thread has none yet: once its MPFR precision is set below,
// whichever module's code it runs creates its buffers at that precision.
//
// An exception must not escape a thread (that is std::terminate), so the first one thrown by
// `action` is kept, the remaining iterations are skipped, and it is rethrown here once every
// thread has joined.
template<typename Callable>
inline void call_multithreaded_operation(int threads, size_t range, Callable&& action){
    if (range == 0){
        return;
    }
    const size_t hardware = std::max(std::thread::hardware_concurrency(), 1u);
    const size_t thread_count = std::min(threads > 0 ? size_t(threads) : hardware, range);

    const mpfr_prec_t prec = mpfr::mpreal::get_default_prec();
    std::atomic<size_t> next{0};
    std::atomic<bool> error_flag{false};
    std::exception_ptr thread_exception = nullptr;
    std::mutex exception_mutex;

    auto worker = [&](){
        ::ode::crafted::set_mpreal_prec(prec);
        for (size_t i = next++; i < range && !error_flag.load(); i = next++){
            try {
                action(i);
            } catch (...) {
                std::lock_guard<std::mutex> lock(exception_mutex);
                if (!error_flag.load()) {
                    thread_exception = std::current_exception();
                    error_flag.store(true);
                }
            }
        }
        // MPFR keeps per-thread caches (e.g. constants used inside exp/log) until asked to free
        // them. Only this thread's own: the shared ones may still be in use by the others.
        mpfr_free_cache2(MPFR_FREE_LOCAL_CACHE);
    };

    {
        std::vector<std::jthread> pool;
        pool.reserve(thread_count);
        try {
            for (size_t k = 0; k < thread_count; k++){
                pool.emplace_back(worker);
            }
        } catch (...) {
            error_flag.store(true); // the threads already started stop at their next iteration,
            throw;                  // and ~jthread joins them
        }
    } // ~jthread joins every thread

    if (thread_exception) {
        std::rethrow_exception(thread_exception);
    }
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

    PyFuncWrapper(const py::capsule& obj, py::ssize_t Nsys, const CArray<py::ssize_t>& output_shape, py::ssize_t Nargs, const std::string& scalar_type);

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

pyshape_t shape_of(const py::iterable& obj);

/**
 * @brief The given object as a C-contiguous py::array, with its shape preserved.
 */
inline py::array as_array(const py::iterable& obj){
    py::array arr = py::array::ensure(obj, py::array::c_style);
    if (!arr){
        throw py::value_error("Could not interpret the given object as an array of numbers");
    }
    return arr;
}

/**
 * @brief The given state as a flat, C-contiguous py::array, whatever shape it arrived in.
 */
inline py::array flatten_state(const py::iterable& obj){
    py::array arr = as_array(obj);
    return arr.ndim() == 1 ? arr : arr.attr("reshape")(-1).cast<py::array>();
}


template<typename T>
inline View<T> as_state(const T* data, size_t nsys, const pyshape_t& shape){
    size_t total = 1;
    for (py::ssize_t dim : shape){
        total *= size_t(dim);
    }
    if (shape.empty() || total != nsys){
        const py::ssize_t flat = py::ssize_t(nsys);
        return View<T>(data, &flat, 1);
    }
    return View<T>(data, shape.data(), shape.size());
}

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
    CArray<T> pyres = rhs_func(t, py::array_t<T>(shape, q), *py_args);

    const T* r = pyres.data();
    for (py::ssize_t i = 0; i < pyres.size(); i++) {
        out[i] = r[i];
    }
}

template<typename T>
inline void jac_pythonic(T* out, const T& t, const T* q, const pyshape_t& shape, const py::tuple& py_args, const py::function& rhs_func) {
    CArray<T> pyres = rhs_func(t, py::array_t<T>(shape, q), *py_args);
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
    std::vector<mpreal_t> arr = to_vector<mpreal_t>(flatten_state(pyres));
    arrcpy(out, arr.data(), arr.size());
}


template<>
inline void jac_pythonic<mpreal_t>(mpreal_t* out, const mpreal_t& t, const mpreal_t* q, const pyshape_t& shape, const py::tuple& py_args, const py::function& rhs_func) {

    py::iterable pyres = rhs_func(t, View<mpreal_t>(q, shape.data(), shape.size()), *py_args);
    std::vector<mpreal_t> arr = to_vector<mpreal_t>(flatten_state(pyres));

    // flatten_state has collapsed the n x n jacobian to one dimension, so the system size
    // comes from the state shape rather than from the result
    py::ssize_t n = 1;
    for (py::ssize_t dim : shape){
        n *= dim;
    }
    if (py::ssize_t(arr.size()) != n*n){
        throw py::value_error("The jacobian of a system of size " + std::to_string(n) + " must have " + std::to_string(n*n) + " elements, not " + std::to_string(arr.size()));
    }

    ndspan::MutView<mpreal_t, ndspan::Layout::F, 0, 0> jm{out, n, n}; // Column-major layout
    ndspan::View2D<mpreal_t> r{arr.data(), n, n};
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