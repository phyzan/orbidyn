#ifndef ORBIDYN_DISPATCHER_HPP
#define ORBIDYN_DISPATCHER_HPP


#include <map>
#include <odecraft/Core/SolverFactory.hpp>
#include "pycast/pycast.hpp"

namespace ode::python{

// All tested working scalar types should be included here
enum class ScalarType : std::uint8_t {
    Float,
    Double,
    LongDouble,
    MPReal
};


static const std::map<std::string, ScalarType> DTYPE_MAP = {
    {"float", ScalarType::Float},
    {"double", ScalarType::Double},
    {"long double", ScalarType::LongDouble},
    {"mpreal", ScalarType::MPReal}
};

inline ScalarType getScalarType(const std::string& dtype){
    auto it = DTYPE_MAP.find(dtype);
    if (it != DTYPE_MAP.end()){
        return it->second;
    }
    else{
        throw std::runtime_error("Unsupported scalar type: " + dtype);
    }
}

inline const char* getScalarType(ScalarType st){
    switch (st){
        case ScalarType::Float:
            return "float";
        case ScalarType::Double:
            return "double";
        case ScalarType::LongDouble:
            return "long double";
        case ScalarType::MPReal:
            return "mpreal";
        default:
            throw std::runtime_error("Invalid ScalarType enum value");
    }
}


template<typename T>
inline ScalarType get_scalar_type(){
    if constexpr (std::is_same_v<T, float>){
        return ScalarType::Float;
    } else if constexpr (std::is_same_v<T, double>){
        return ScalarType::Double;
    } else if constexpr (std::is_same_v<T, long double>){
        return ScalarType::LongDouble;
    } else if constexpr (std::is_same_v<T, mpreal_t>){
        return ScalarType::MPReal;
    } else{
        static_assert(false, "Unsupported scalar type T");
    }
}


inline const char* integrator_name(Integrator method){
    
    switch (method){
        case Integrator::Euler: return "Euler";
        case Integrator::RK4: return "RK4";
        case Integrator::RK23: return "RK23";
        case Integrator::RK45: return "RK45";
        case Integrator::DOP853: return "DOP853";
        case Integrator::BDF: return "BDF";
        default: throw std::runtime_error("Unknown integrator enum value");
    }
}


inline Integrator getIntegrator(const char* name){
    if (strcmp(name, "Euler") == 0){
        return Integrator::Euler;
    }else if (strcmp(name, "RK4") == 0){
        return Integrator::RK4;
    }else if (strcmp(name, "RK23") == 0){
        return Integrator::RK23;
    }else if (strcmp(name, "RK45") == 0){
        return Integrator::RK45;
    }else if (strcmp(name, "DOP853") == 0){
        return Integrator::DOP853;
    }else if (strcmp(name, "BDF") == 0){
        return Integrator::BDF;
    }else{
        throw std::runtime_error("Unknown integrator name");
    }
}

inline Integrator getIntegrator(const std::string& name){
    return getIntegrator(name.c_str());
}

} // namespace ode::python

#endif // ORBIDYN_DISPATCHER_HPP