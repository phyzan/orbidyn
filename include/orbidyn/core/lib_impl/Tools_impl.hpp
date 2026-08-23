#ifndef ORBIDYN_TOOLS_IMPL_HPP
#define ORBIDYN_TOOLS_IMPL_HPP


#include "../lib/Tools.hpp"
#include "../pycast/pycast.hpp" // IWYU pragma: keep


namespace ode::python{


namespace py = pybind11;

template<typename T>
std::vector<T> to_vector(const py::iterable &obj) {

    // Get Python iterable length if possible (optional, for efficiency)
    size_t len = py::len(obj);
    std::vector<T> res(len);
    int i = 0;
    for (auto item : obj) {
        res[i++] = py::cast<T>(item);
    }
    return res;
}

template<typename Int>
std::vector<Int> getShape(const py::ssize_t& dim1, const pyshape_t& shape){
    std::vector<Int> result;
    result.reserve(1 + shape.size()); // Pre-allocate memory for efficiency
    result.push_back(Int(dim1));        // Add the first element
    for (size_t i=0; i<shape.size(); i++){
        result.push_back(shape[i]);
    }
    return result;
}

template<typename Scalar, typename ArrayType>
py::array_t<Scalar> to_numpy(const ArrayType& array, const pyshape_t& state_shape){
    if (state_shape.size() == 0){
        py::array_t<Scalar> res(pyshape_t{static_cast<py::ssize_t>(array.size())}, array.data());
        return res;
    }
    else{
        py::array_t<Scalar> res(state_shape, array.data());
        return res;
    }
}

template<typename T>
py::array_t<T> array(T* data, const pyshape_t& shape){
    py::capsule capsule = py::capsule(data, [](void* r){T* d = reinterpret_cast<T*>(r); delete[] d;});
    return py::array_t<T>(shape, data, capsule);
}


//===========================================================================================
//                                      PyFuncWrapper
//===========================================================================================


} // namespace ode::python

#endif // ORBIDYN_TOOLS_IMPL_HPP