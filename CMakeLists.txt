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


option(DEBUG "Build unoptimised with assertions enabled (-O0 -g3) instead of the optimized release build" OFF)

# CMAKE_BUILD_TYPE is pinned to Release by pyproject.toml, and Release carries its own
# -O3 -DNDEBUG in CMAKE_CXX_FLAGS_RELEASE. Those land ahead of the flags assembled below, so a
# debug build would compile with `-O3 -DNDEBUG ... -O0 -UNDEBUG` and rely on the later
# occurrence winning. Switching the configuration outright is what makes the command line say
# what it means -- and odecraft keys its own debug build on the same two things.
if(DEBUG)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(ORBIDYN_NATIVE_ARCH)
    set(ORBIDYN_ARCH_FLAG march=native)
else()
    set(ORBIDYN_ARCH_FLAG march=x86-64)
endif()


if(DEBUG)
    set(ORBIDYN_CODEGEN_FLAGS -O0 -g3 -UNDEBUG)
else()
    set(ORBIDYN_CODEGEN_FLAGS
        -O3 -DNDEBUG -g0 -fno-math-errno -fno-trapping-math
        -ffunction-sections -fdata-sections
    )
    set(ORBIDYN_LINK_OPTIONS -Wl,--gc-sections -Wl,--as-needed)
endif()
list(APPEND ORBIDYN_CODEGEN_FLAGS -${ORBIDYN_ARCH_FLAG} -fvisibility=hidden)

# Warnings are orbidyn's own business and are deliberately not passed on to generated code.
set(ORBIDYN_COMPILE_OPTIONS -Wall -Wextra -Wno-unused-parameter ${ORBIDYN_CODEGEN_FLAGS})

#===========================================================================================
#                           Dependencies (vendored as git submodules)
#===========================================================================================

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
add_subdirectory(external/pybind11)

set(ODECRAFT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LAZEX_ENABLE_MPREAL ON CACHE BOOL "" FORCE)
# No FORCE: the rounding mode is the caller's to pick, and an empty value drops the macro
# so mpreal reads mpfr::mpreal::get_default_rnd() at runtime instead.
set(LAZEX_MPFR_RND "MPFR_RNDN" CACHE STRING "Rounding mode for mpreal arithmetic")

# orbidyn is built entirely on the pre-compiled ode::crafted interface, so the library that
# ships those instantiations has to be built. odecraft defaults this OFF when it is pulled in
# via add_subdirectory, on the assumption that the parent only wants the header-only `ode`.
set(ODECRAFT_BUILD_CRAFTED ON CACHE BOOL "" FORCE)


set(ODECRAFT_USE_LAZY_MPREAL OFF CACHE BOOL
    "Compile ode::crafted's mpreal_t as lazex::LazyType<mpfr::mpreal> instead of plain mpfr::mpreal")
add_subdirectory(external/odecraft)

# Enabled only after the vendored subdirectories above, so compile_commands.json covers
# orbidyn's own targets without being swamped by pybind11/odecraft/qhull's internal ones
# (CMAKE_EXPORT_COMPILE_COMMANDS is a plain variable: a subdirectory only inherits it if
# it was already ON when add_subdirectory() was called).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# clangd looks for the database next to the source root or in <root>/build/, never in
# build/{wheel_tag}/ where scikit-build-core puts it (see build-dir in pyproject.toml).
# Skipped when vendored via add_subdirectory(), so it can't clobber a parent project's link.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json"
         "${CMAKE_SOURCE_DIR}/compile_commands.json" SYMBOLIC)
endif()

find_package(OpenMP)

#===========================================================================================
#                           orbidyn glue libraries (python/pyode bindings on top of odecraft)
#===========================================================================================

add_library(orbidyn_headers INTERFACE)
target_include_directories(orbidyn_headers INTERFACE include)
target_link_libraries(orbidyn_headers INTERFACE odecraft::crafted lazex::mpfr pybind11::pybind11 Python::Module)
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
#                           Build configuration for JIT-compiled code
#===========================================================================================
# ODECRAFT_USE_LAZY_MPREAL and friends are preprocessor macros the odecraft, xdiff and lazex
# headers read, and that one decides a *type*. CMake gives them to the targets below as usage
# requirements, but JIT-compiled user code never goes through CMake: numiphy invokes the
# compiler itself using OdeSystem.compile_flags(). Disagree, and the mismatch surfaces as an
# undefined reference to a mangled name, or as two incompatible definitions of one type.
#
# They cannot be hardcoded on the Python side either, precisely because the caller may flip
# them. So write out what this build actually used and have compile_flags() read it back.
# Taken from the real target at generate time, so a new option needs no mirroring here.
file(GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_buildconfig.py
    CONTENT "# Generated by CMake. Do not edit.
# How orbidyn's static libraries were compiled. JIT-compiled user code links against them, so
# it has to be given the same flags. See OdeSystem.compile_flags() in symbolic.py.
_DEFS = \"$<JOIN:$<TARGET_PROPERTY:orbidyn_pytools,COMPILE_DEFINITIONS>,;>\"
COMPILE_DEFINITIONS = [d for d in _DEFS.split(';') if d]

# Optimisation, debug and architecture flags, minus the leading dash: the compiler driver
# numiphy uses adds it back.
_FLAGS = \"${ORBIDYN_CODEGEN_FLAGS};${ORBIDYN_LINK_OPTIONS}\"
COMPILE_FLAGS = [f.lstrip('-') for f in _FLAGS.split(';') if f]
"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/_buildconfig.py DESTINATION orbidyn)

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

# odecraft_crafted ships with them: JIT-compiled user code includes <orbidyn/orbidyn.hpp>,
# which is built on ode::crafted, so it needs the instantiations at link time just as the
# static libraries above do. See OdeSystem.get_link_names() in python/orbidyn/symbolic.py.
install(TARGETS orbidyn_pytools orbidyn_integrators orbidyn_eventhandling orbidyn_history
                orbidyn_lowlevelode orbidyn_chaos orbidyn_spatial orbidyn_sampling
                odecraft_crafted
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
install(DIRECTORY external/odecraft/external/xdiff/external/lazex/include/
    DESTINATION orbidyn/include
    FILES_MATCHING PATTERN "*.hpp"
)
install(FILES external/odecraft/external/xdiff/external/lazex/external/mpreal/mpreal.h
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
