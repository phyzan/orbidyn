#ifndef ORBIDYN_SAMPLING_HPP
#define ORBIDYN_SAMPLING_HPP


#include "Spatial.hpp"

namespace ode::python {


struct PyScalarField {

    // returns double (not array), as this is a scalar field.
    static py::object py_value_at(const VirtualNdInterpolator& self, const py::args& x);
}; // struct PyScalarField



class PyRegScalarField : public RegularGridInterpolator, public PyScalarField {

public:

    using RGBase = RegularGridInterpolator;

    static c_array_t parse_values(const c_array_t& values, const py::args& py_grid);
    
    PyRegScalarField(const c_array_t& values, const py::args& py_grid);
    

    // Keep in the header file for easier instantiation from dynamically compiled ODE code
    // regardless of the number of dimensions, without needing to explicitly instantiate for each dimension in the precompiled library.
    template<typename... Scalar>
    double operator()(Scalar... x) const{
        assert(sizeof...(Scalar) == this->ndim() && "Number of coordinates must match number of dimensions of the field");
        double coords[] = {double(x)...};
        double out;
        if (!this->interp(&out, coords)){
            return std::numeric_limits<double>::quiet_NaN();
        }else {
            return out;
        }
    }

}; // class PyRegScalarField



class PyScatteredField : public ScatteredNdInterpolator, public PyScalarField {


public:

    using InterpBase = ScatteredNdInterpolator;

    // Python signature is ScatteredField(x: np.ndarray (npoints, ndim), values: np.ndarray (npoints,)
    PyScatteredField(const c_array_t& x, const c_array_t& values);

    PyScatteredField(const PyDelaunay& tri, const c_array_t& values);

    py::object py_delaunay() const;

    py::object py_points() const;

    py::object py_values() const;

    static std::nullptr_t parse_values(const c_array_t& values);

    // Keep in the header file for easier instantiation from dynamically compiled ODE code
    // regardless of the number of dimensions, without needing to explicitly instantiate for each dimension in the precompiled library.
    template<typename... Scalar>
    double operator()(Scalar... x) const{
        double coords[] = {double(x)...};
        assert(sizeof...(Scalar) == this->ndim() && "Number of coordinates must match number of dimensions of the field");
        double out;
        if (!this->interp(&out, coords)){
            return std::numeric_limits<double>::quiet_NaN();
        }else {
            return out;
        }
    }

private:

    PyScatteredField(std::nullptr_t, const c_array_t& x, const c_array_t& values);

    PyScatteredField(std::nullptr_t, const PyDelaunay& tri, const c_array_t& values);

}; // class PyScatteredField




struct PyVecField {

    // returns array, as this is a vector field.

    using CLS = VirtualVectorField;

    static void check_coords(const CLS& self, const double* coords);

    static py::object py_streamline(const CLS& self, const c_array_t& q0, double length, double rtol, double atol, double min_step, const py::object& max_step, double stepsize, int direction, const py::object& t_eval, const py::str& method, bool normalized);

    static py::object py_streamline_ode(const CLS& self, const c_array_t& q0, double rtol, double atol, double min_step, const py::object& max_step, double stepsize, int direction, const py::str& method, bool normalized);
    
}; // class PyVecField


struct PyRegVecField {

    using RGBase = PyRegGridInterp;
    using VFBase = PyVecField;

    using CLS = RegularVectorField;

    // ============================= Python interface =============================
    
    static CLS init_main(const c_array_t& values, const py::args& py_grid, const std::string& coord_type);

    static py::object py_streamplot_data(const CLS& self, double max_length, int density, double ds, double rtol, double atol, double min_step, const py::object& max_step, double stepsize, const py::str& method);

    static py::object component(const CLS& self, int i);

    // ============================================================================

    static void parse_values(const c_array_t& values, const py::args& py_grid);

}; // class PyRegVecField


struct PyScatVecField {

    using SCBase = PyScatteredInterp;
    using VFBase = PyVecField;

    using CLS = ScatteredVectorField;


public:

    static CLS init(const c_array_t& x, const c_array_t& values);

    static CLS init_tri(const PyDelaunay& tri, const c_array_t& values);

    static void parse_values( const c_array_t& values);

}; // class PyScatVecField

} // namespace ode::python

#endif // ORBIDYN_SAMPLING_HPP