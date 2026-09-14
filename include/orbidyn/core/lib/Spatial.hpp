#ifndef ORBIDYN_SPATIAL_HPP
#define ORBIDYN_SPATIAL_HPP


#include "Tools.hpp" // IWYU pragma: keep

namespace ode::python {

/// @brief A C-contiguous double array, which is what every interpolator constructor wants.
using c_array_t = py::array_t<double, py::array::c_style | py::array::forcecast>;

/**
 * @brief View @p arr as the values container the compiled constructors take.
 *
 * odecraft compiles those constructors for exactly one container, crafted::field_values_t;
 * handing them a py::array_t instead would need the constructor template instantiated here,
 * which is what linking against the compiled interface is meant to avoid. odecraft also has
 * no business knowing what a py::array_t is.
 *
 * Non-owning: @p arr must outlive the constructor call, so keep it in a named local.
 */
field_values_t as_field_values(const c_array_t& arr);



struct PyNdInterp {

    static py::object py_value_at(const VirtualNdInterpolator& self, const py::args& x);

    static int ndim(const VirtualNdInterpolator& self);

}; // struct PyNdInterp



struct PyRegGridInterp {

    using CLS = RegularGridInterpolator;

    // ======================= Python interface =====================================
    static CLS init_main(const py::array_t<double>& values, const py::args& py_grid);

    static py::object get_values(const CLS& self);

    static py::tuple get_grid(const CLS& self);

    // ==============================================================================

    static CLS init(const py::array_t<double>& values, const std::vector<Array1D<double>>& grid, bool coord_axis_first);

    static CLS init(const py::array_t<double>& values, const py::args& py_grid, bool coord_axis_first);

    static std::vector<Array1D<double>> parse_args(const py::array_t<double>& values, const py::args& py_grid, bool coord_axis_first);

}; // class PyRegGridInterp



class PyDelaunay {

    using Base = DelaunayTri;

public:

    PyDelaunay(const py::array_t<double>& x);

    PyDelaunay(TriPtr tri);

    py::object py_points() const;

    int py_ndim() const;
    int py_npoints() const;
    int py_nsimplices() const;
    int py_find_simplex(const py::array_t<double>& point) const;

    //returns array of shape (ndim+1,), or None if point is out of bounds.
    py::object py_get_simplex(const py::array_t<double>& point) const;
    
    // array of shape (nsimplices, ndim+1) containing the indices of the points that form each simplex
    py::object py_get_simplices() const;

    double total_volume() const;

    TriPtr tri() const;

    // ================ Pickling ===================
    py::dict    py_get_state() const;
    
    static PyDelaunay py_set_state(const py::dict& state);
    // =============================================
    

private:

    PyDelaunay(std::nullptr_t, const py::array_t<double>& x);

    PyDelaunay() = default; //for pickling, not called from outside

    static std::nullptr_t parse_args(const py::array_t<double>& x);

    TriPtr tri_;
    mutable bool volume_is_cached_ = false;
    mutable double cached_total_volume_ = 0;

}; // class PyDelaunay


struct PyScatteredInterp {

    using CLS = ScatteredNdInterpolator;

    // Python signature is ScatteredNdInterpolator(x: np.ndarray (npoints, ndim), values: np.ndarray (npoints, ...)), where
    static CLS init_main(const py::array_t<double>& x, const py::array_t<double>& values);

    static CLS init_tri(const PyDelaunay& tri, const py::array_t<double>& values);

    static CLS init(const py::array_t<double>& x, const py::array_t<double>& values, bool coord_axis_first);

    static CLS init(const PyDelaunay& tri, const py::array_t<double>& values, bool coord_axis_first);

    static py::object py_delaunay_obj(const CLS& self);

    static int ndim(const CLS& self);

    static py::object py_points(const CLS& self);

    static py::object py_values(const CLS& self);

    // Internal interface

    static CLS init(std::nullptr_t, const py::array_t<double>& x, const py::array_t<double>& values, bool coord_axis_first);

    static CLS init(std::nullptr_t, const PyDelaunay& tri, const py::array_t<double>& values, bool coord_axis_first);

    static std::nullptr_t parse_args(const py::array_t<double>& x, const py::array_t<double>& values, bool coord_axis_first);
    static std::nullptr_t parse_tri_args(const PyDelaunay& tri, const py::array_t<double>& values, bool coord_axis_first);


}; // struct PyScatteredInterp


} // namespace ode::python


#endif // ORBIDYN_SPATIAL_HPP