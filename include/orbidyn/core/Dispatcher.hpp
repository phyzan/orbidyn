#ifndef ORBIDYN_DISPATCHER_HPP
#define ORBIDYN_DISPATCHER_HPP


#include <map>
#include "Crafted.hpp"
#include "pycast/pycast.hpp" // IWYU pragma: keep


#define ORBIDYN_SCALARS float, double, long double, mpreal_t

namespace ode::python{

// ========== Scalar types ==========
enum class ScalarType : std::uint8_t {
    Float,
    Double,
    LongDouble,
    MPReal
};

// =================================== Mapping logic =====================================
// ======================= ScalarType -> T -> string -> ScalarType =======================


// ScalarType -> T
template<typename ReturnType, typename Callable, typename... Args>
inline constexpr ReturnType dispatch_scalar_type(ScalarType scalar_type, Callable&& callable, Args&&... args){
    switch (scalar_type){
        case ScalarType::Float:
            return callable.template operator()<float>(std::forward<Args>(args)...);
        case ScalarType::Double:
            return callable.template operator()<double>(std::forward<Args>(args)...);
        case ScalarType::LongDouble:
            return callable.template operator()<long double>(std::forward<Args>(args)...);
        case ScalarType::MPReal:
            return callable.template operator()<mpreal_t>(std::forward<Args>(args)...);
        default:
            throw std::runtime_error("Unknown scalar type");
    }
}

// T -> ScalarType
template<typename T>
inline constexpr ScalarType getScalarType(){
    if constexpr (std::is_same_v<T, float>) {
        return ScalarType::Float;
    } else if constexpr (std::is_same_v<T, double>) {
        return ScalarType::Double;
    } else if constexpr (std::is_same_v<T, long double>) {
        return ScalarType::LongDouble;
    } else if constexpr (std::is_same_v<T, mpreal_t>) {
        return ScalarType::MPReal;
    } else {
        static_assert(false, "Unsupported scalar type T");
    }
}

// T -> string
template<typename T>
constexpr const char* getScalarTypeName() {
    if constexpr (std::is_same_v<T, float>) {
        return "float";
    } else if constexpr (std::is_same_v<T, double>) {
        return "double";
    } else if constexpr (std::is_same_v<T, long double>) {
        return "long double";
    } else if constexpr (std::is_same_v<T, mpreal_t>) {
        return "mpreal";
    } else {
        static_assert(false, "Unsupported scalar type T");
    }
}


// std::string -> ScalarType
inline ScalarType getScalarTypefromStr(const std::string& dtype) {

    static std::map<std::string, ScalarType> DTYPE_MAP = {
        {"float", ScalarType::Float},
        {"double", ScalarType::Double},
        {"long double", ScalarType::LongDouble},
        {"mpreal", ScalarType::MPReal}
    };

    auto it = DTYPE_MAP.find(dtype);
    if (it == DTYPE_MAP.end())
        throw std::runtime_error("Unsupported scalar type: " + dtype);
    return it->second;
}



// ScalarType -> string
inline constexpr const char* getScalarTypeName(ScalarType st){
    return dispatch_scalar_type<const char*>(st, []<typename T>(){
        return getScalarTypeName<T>();
    });
}

} // namespace ode::python

#endif // ORBIDYN_DISPATCHER_HPP