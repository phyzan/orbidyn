cmake_minimum_required(VERSION 3.15...3.29)
project(orbidyn LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(ORBIDYN_NATIVE_ARCH "Enable -march=native instead of -march=x86-64" OFF)

# These flags must match OdeSystem.compile_flags() in python/orbidyn/symbolic.py, which
# JIT-compiles user code against the static libraries built below.
set(ORBIDYN_COMPILE_OPTIONS -Wall -Wextra -Wno-unused-parameter)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND ORBIDYN_COMPILE_OPTIONS
        -O3 -DNDEBUG -g0 -fno-math-errno -fno-trapping-math
        -ffunction-sections -fdata-sections
    )
    set(ORBIDYN_LINK_OPTIONS -Wl,--gc-sections -Wl,--as-needed)
endif()
if(ORBIDYN_NATIVE_ARCH)
    list(APPEND ORBIDYN_COMPILE_OPTIONS -march=native)
else()
    list(APPEND ORBIDYN_COMPILE_OPTIONS -march=x86-64)
endif()

#===========================================================================================
#                           Dependencies (vendored as git submodules)
#===========================================================================================

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
add_subdirectory(external/pybind11)

set(ODECRAFT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LAZY_ENABLE_MPREAL ON CACHE BOOL "" FORCE)
set(LAZY_MPFR_RND "MPFR_RNDN" CACHE STRING "" FORCE)
add_subdirectory(external/odecraft)

# Enabled only after the vendored subdirectories above, so compile_commands.json covers
# orbidyn's own targets without being swamped by pybind11/odecraft/qhull's internal ones
# (CMAKE_EXPORT_COMPILE_COMMANDS is a plain variable: a subdirectory only inherits it if
# it was already ON when add_subdirectory() was called).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenMP)

#===========================================================================================
#                           orbidyn glue libraries (python/pyode bindings on top of odecraft)
#===========================================================================================

add_library(orbidyn_headers INTERFACE)
target_include_directories(orbidyn_headers INTERFACE include)
target_link_libraries(orbidyn_headers INTERFACE odecraft::odecraft lazy::mpfr pybind11::pybind11 Python::Module)
target_compile_options(orbidyn_headers INTERFACE ${ORBIDYN_COMPILE_OPTIONS})
target_link_options(orbidyn_headers INTERFACE ${ORBIDYN_LINK_OPTIONS})
if(OpenMP_FOUND)
    target_link_libraries(orbidyn_headers INTERFACE OpenMP::OpenMP_CXX)
endif()

function(orbidyn_static_lib name)
    add_library(orbidyn_${name} STATIC ${ARGN})
    target_link_libraries(orbidyn_${name} PUBLIC orbidyn_headers)
endfunction()

orbidyn_static_lib(pytools        src/staticlib/Tools.cpp)
orbidyn_static_lib(eventhandling  src/staticlib/EventHandling.cpp)
target_link_libraries(orbidyn_eventhandling PUBLIC orbidyn_pytools)

orbidyn_static_lib(integrators    src/staticlib/Integrators.cpp src/staticlib/SubIntegrators.cpp)
target_link_libraries(orbidyn_integrators PUBLIC orbidyn_eventhandling orbidyn_pytools)

orbidyn_static_lib(history        src/staticlib/History.cpp)
target_link_libraries(orbidyn_history PUBLIC orbidyn_integrators)

orbidyn_static_lib(lowlevelode    src/staticlib/LowLevelOde.cpp)
target_link_libraries(orbidyn_lowlevelode PUBLIC orbidyn_integrators orbidyn_eventhandling orbidyn_history orbidyn_pytools)

orbidyn_static_lib(chaos          src/staticlib/Chaos.cpp)
target_link_libraries(orbidyn_chaos PUBLIC orbidyn_lowlevelode orbidyn_integrators orbidyn_eventhandling orbidyn_history orbidyn_pytools)

orbidyn_static_lib(spatial        src/staticlib/Spatial.cpp)
target_link_libraries(orbidyn_spatial PUBLIC orbidyn_lowlevelode orbidyn_integrators orbidyn_history orbidyn_pytools)

orbidyn_static_lib(sampling       src/staticlib/Sampling.cpp)
target_link_libraries(orbidyn_sampling PUBLIC orbidyn_spatial)

#===========================================================================================
#                           Python extension modules
#===========================================================================================

pybind11_add_module(integrators src/bindings/integrators.cpp)
target_link_libraries(integrators PRIVATE orbidyn_integrators)

pybind11_add_module(eventhandling src/bindings/eventhandling.cpp)
target_link_libraries(eventhandling PRIVATE orbidyn_eventhandling)

pybind11_add_module(history src/bindings/history.cpp)
target_link_libraries(history PRIVATE orbidyn_history)

pybind11_add_module(lowlevelode src/bindings/lowlevelode.cpp)
target_link_libraries(lowlevelode PRIVATE orbidyn_lowlevelode)

pybind11_add_module(chaos src/bindings/chaos.cpp)
target_link_libraries(chaos PRIVATE orbidyn_chaos)

pybind11_add_module(spatial src/bindings/spatial.cpp)
target_link_libraries(spatial PRIVATE orbidyn_spatial)

pybind11_add_module(sampling src/bindings/sampling.cpp)
target_link_libraries(sampling PRIVATE orbidyn_sampling)

#===========================================================================================
#                           Install (into the orbidyn package inside the wheel)
#===========================================================================================

install(TARGETS integrators eventhandling history lowlevelode chaos spatial sampling
    LIBRARY DESTINATION orbidyn
    RUNTIME DESTINATION orbidyn
)

install(TARGETS orbidyn_pytools orbidyn_integrators orbidyn_eventhandling orbidyn_history
                orbidyn_lowlevelode orbidyn_chaos orbidyn_spatial orbidyn_sampling
    ARCHIVE DESTINATION orbidyn/lib
)

# Merged into one include/ tree so JIT-compiled user code (numiphy's compile_tools.py,
# via a single -I<pkg>/include) resolves headers the exact same way the CMake build does:
#   <pkg>/include/orbidyn/...    (orbidyn::orbidyn_headers  -I root is "include")
#   <pkg>/include/odecraft/...   (odecraft::odecraft         -I root is "external/odecraft/include")
# pybind11's headers aren't installed here: the vendored submodule (external/pybind11) is
# only used to build orbidyn itself; JIT compilation gets pybind11 headers separately from
# the pip-installed pybind11 package via pybind11.get_include() (numiphy is required to
# depend on pybind11 for this).
install(DIRECTORY include/
    DESTINATION orbidyn/include
    FILES_MATCHING PATTERN "*.hpp"
)
install(DIRECTORY external/odecraft/include/
    DESTINATION orbidyn/include
    FILES_MATCHING PATTERN "*.hpp"
)

# odecraft::odecraft also transitively exposes these vendored dependencies' include dirs
# (see external/odecraft/CMakeLists.txt), which the normal CMake build picks up via
# target_link_libraries propagation. JIT-compiled user code has no such propagation -
# it only gets -I<pkg>/include (OdeSystem.include_dirs() in symbolic.py) - so those headers
# must be merged in here too, or JIT compiles that touch sampling (e.g. RegularScalarField,
# which pulls in odecraft/ode/Tools.hpp's <xdiff/xdiff.hpp> etc.) fail with a missing header.
install(DIRECTORY external/odecraft/external/xdiff/include/
    DESTINATION orbidyn/include
    FILES_MATCHING PATTERN "*.hpp"
)
install(DIRECTORY external/odecraft/external/xdiff/external/lazy/include/
    DESTINATION orbidyn/include
    FILES_MATCHING PATTERN "*.hpp"
)
install(FILES external/odecraft/external/xdiff/external/lazy/external/mpreal/mpreal.h
    DESTINATION orbidyn/include
)
install(DIRECTORY external/odecraft/external/ndspan/include/
    DESTINATION orbidyn/include
    FILES_MATCHING PATTERN "*.hpp"
)
install(DIRECTORY external/odecraft/external/polybox/include/
    DESTINATION orbidyn/include
    FILES_MATCHING PATTERN "*.hpp"
)
install(DIRECTORY external/odecraft/external/qhull/src/libqhull_r/
    DESTINATION orbidyn/include/libqhull_r
    FILES_MATCHING PATTERN "*.h"
)
