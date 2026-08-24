#include <odecraft/Core/Events_impl.hpp>
#include <orbidyn/core/lib/EventHandling.hpp>
#include <orbidyn/core/lib_impl/Tools_impl.hpp>


namespace ode::python{

    std::string name_;
    size_t nsys_;
    size_t nargs_;
    py::function py_mask_;
    RawPyRhs_t mask_{};
    bool delay_mask_;
    bool is_lowlevel_ = false;
    bool is_masked_ = false;





//===========================================================================================
//                                      PyEvent
//===========================================================================================

PyEvent::PyEvent(std::string name, py::object mask, bool delay_mask, const std::string& scalar_type, size_t Nsys, size_t Nargs) : DtypeDispatcher(scalar_type), name_(std::move(name)), nsys_(Nsys), nargs_(Nargs), delay_mask_(delay_mask){
    this->is_lowlevel_ = true;
    this->is_masked_ = true;
    if (py::isinstance<py::capsule>(mask)){
        this->mask_ = dispatch_scalar_type<RawPyRhs_t>(getScalarTypefromStr(scalar_type), [&]<typename T>(){
            return open_capsule<raw_pyrhs_t<T>>(mask);
        });
        this->requires_check_sizes_ = true;
    } else if (py::isinstance<py::function>(mask) && !mask.is_none()){
        this->py_mask_ = std::move(mask);
        this->is_lowlevel_ = false;
    } else {
        this->is_masked_ = false;
    }
}



py::str PyEvent::name() const{
    return name_;
}

bool PyEvent::mask_delayed() const {
    return delay_mask_;
}

bool PyEvent::is_lowlevel() const{
    return this->is_lowlevel_;
}


void PyEvent::check_sizes(size_t Nsys, size_t Nargs) const{
    if (this->requires_check_sizes_){
        if (nsys_ != Nsys){
            throw py::value_error("The event named \""+this->name_+"\" can only be applied on an ode of system size "+std::to_string(nsys_)+", not "+std::to_string(Nsys));
        } else if (nargs_ != Nargs){
            throw py::value_error("The event named \""+this->name_+"\" can only accept "+std::to_string(nargs_)+" extra args, not "+std::to_string(Nargs));
        }
    }
}

//===========================================================================================
//                                      PyPrecEvent
//===========================================================================================

PyPrecEvent::PyPrecEvent(std::string name, py::object when, int dir, py::object mask, bool delay_mask, py::object event_tol, const std::string& scalar_type, size_t Nsys, size_t Nargs) : PyEvent(std::move(name), std::move(mask), delay_mask, scalar_type, Nsys, Nargs), dir_(sgn(dir)), event_tol_(std::move(event_tol)){
    if (py::isinstance<py::capsule>(when)){
        this->obj_fun_ = dispatch_scalar_type<RawPyObjFun_t>(getScalarTypefromStr(scalar_type), [&]<typename T>(){
            return open_capsule<raw_pyobjfun_t<T>>(when);
        });
        this->requires_check_sizes_ = true;
    } else if (py::isinstance<py::function>(when)){
        this->py_objfun_ = std::move(when);
        this->is_lowlevel_ = false;
    } else {
        throw py::value_error("The objective function of PreciseEvent must be a callable function");
    }
}



event_generic_t PyPrecEvent::toEvent(const pyshape_t& shape, py::tuple pyargs) const{

    if (this->is_lowlevel()){
        for (const py::handle& arg : pyargs){
            if (PyNumber_Check(arg.ptr())==0){
                throw py::value_error("All args must be numbers");
            }
        }
        // obj_fun_ is not null
        return std::visit([=, this]<typename T>(raw_pyobjfun_t<T> objfun) -> event_generic_t {
            std::vector<T> args = to_vector<T>(pyargs);
            if (this->is_masked_){
                // also mask is not null
                assert((std::holds_alternative<raw_pyrhs_t<T>>(this->mask_)) && "objfun and mask have incompatible scalar types");
                return make_event<T, PreciseEvent>(
                    this->name(),
                    [=](const T& t, const T* q){
                        return objfun(t, q, args.data());
                    },
                    this->event_tol_.cast<T>(),
                    this->dir_,
                    this->mask_compiled(pyargs, std::get<raw_pyrhs_t<T>>(this->mask_)),
                    this->mask_delayed()
                );
            } else {
                return make_event<T, PreciseEvent>(
                    this->name(),
                    [=](const T& t, const T* q){
                        return objfun(t, q, args.data());
                    },
                    this->event_tol_.cast<T>(),
                    this->dir_
                );
            }
        }, this->obj_fun_);
    } else if (this->is_masked_){
        if (std::visit([](auto objfun){return objfun != nullptr;}, this->obj_fun_)){
            // objfun lowlevel, mask pythonic
            return std::visit([=, this]<typename T>(raw_pyobjfun_t<T> objfun) -> event_generic_t {
                std::vector<T> args = to_vector<T>(pyargs);
                return make_event<T, PreciseEvent>(
                    this->name(),
                    [=](const T& t, const T* q){
                        return objfun(t, q, args.data());
                    },
                    this->event_tol_.cast<T>(),
                    this->dir_,
                    this->mask_pythonic<T>(shape, pyargs),
                    this->mask_delayed()
                );
            }, this->obj_fun_);
        } else if (std::visit([](auto maskfunc){return maskfunc != nullptr;}, this->mask_)) {
            // objfun pythonic, mask lowlevel
            return std::visit([=, this]<typename T>(raw_pyrhs_t<T> mask) -> event_generic_t {
                std::vector<T> args = to_vector<T>(pyargs);
                return make_event<T, PreciseEvent>(
                    this->name(),
                    [=, objfun=this->py_objfun_](const T& t, const T* q){
                        return objfun_pythonic(t, q, shape, pyargs, objfun);
                    },
                    this->event_tol_.cast<T>(),
                    this->dir_,
                    this->mask_compiled(pyargs, mask),
                    this->mask_delayed()
                );
            }, this->mask_);
        } else {
            // both pythonic
            return dispatch_scalar_type<event_generic_t>(
                this->scalar_type,
                [=, this]<typename T>(){
                    return make_event<T, PreciseEvent>(
                        this->name(),
                        [=, objfun=this->py_objfun_](const T& t, const T* q){
                            return objfun_pythonic(t, q, shape, pyargs, objfun);
                        },
                        this->event_tol_.cast<T>(),
                        this->dir_,
                        this->mask_pythonic<T>(shape, pyargs),
                        this->mask_delayed()
                    );
                }
            );
        }
    } else {
        // no mask, objfun pythonic
        return dispatch_scalar_type<event_generic_t>(
            this->scalar_type,
            [=, this]<typename T>(){
                return make_event<T, PreciseEvent>(
                    this->name(),
                    [=, objfun=this->py_objfun_](const T& t, const T* q){
                        return objfun_pythonic(t, q, shape, pyargs, objfun);
                    },
                    this->event_tol_.cast<T>(),
                    this->dir_
                );
            }
        );
    }
}

py::object PyPrecEvent::event_tol() const {
    return event_tol_;
}

//===========================================================================================
//                                      PyPerEvent
//===========================================================================================

PyPerEvent::PyPerEvent(std::string name, py::object period, py::object mask, bool delay_mask, const std::string& scalar_type, size_t Nsys, size_t Nargs):PyEvent(std::move(name), std::move(mask), delay_mask, scalar_type, Nsys, Nargs), period_(std::move(period)) {}

event_generic_t PyPerEvent::toEvent(const pyshape_t& shape, py::tuple pyargs) const {

    if (this->is_lowlevel()){
        for (const py::handle& arg : pyargs){
            if (PyNumber_Check(arg.ptr())==0){
                throw py::value_error("All args must be numbers");
            }
        }
    }

    if (this->is_masked_ && std::visit([](auto maskfunc){return maskfunc != nullptr;}, this->mask_)){
        // mask lowlevel
        return dispatch_scalar_type<event_generic_t>(
            this->scalar_type,
            [this, pyargs]<typename T>(){
                assert((std::holds_alternative<raw_pyrhs_t<T>>(this->mask_)) && "Periodic event and its mask have incompatible scalar types");
                return make_event<T, PeriodicEvent>(
                    this->name(),
                    this->period_.cast<T>(),
                    this->mask_compiled(pyargs, std::get<raw_pyrhs_t<T>>(this->mask_)),
                    this->mask_delayed()
                );
            }
        );
    } else if (this->is_masked_ && std::visit([](auto maskfunc){return maskfunc == nullptr;}, this->mask_)){
        // mask pythonic
        return dispatch_scalar_type<event_generic_t>(
            this->scalar_type,
            [=, this]<typename T>(){
                return make_event<T, PeriodicEvent>(
                    this->name(),
                    this->period_.cast<T>(),
                    this->mask_pythonic<T>(shape, pyargs),
                    this->mask_delayed()
                );
            }
        );
    } else {
        // no mask
        return dispatch_scalar_type<event_generic_t>(
            this->scalar_type,
            [this]<typename T>(){
                return make_event<T, PeriodicEvent>(
                    this->name(),
                    this->period_.cast<T>(),
                    nullptr,
                    this->mask_delayed()
                );
            }
        );
    }
}

py::object PyPerEvent::period() const{
    return period_;
}


//===========================================================================================
//                                      Helper functions
//===========================================================================================


bool all_are_lowlevel(const py::iterable& events){
    if (events.is_none()){
        return true;
    }
    for (py::handle item : events){
        if (!item.cast<PyEvent&>().is_lowlevel()){
            return false;
        }
    }
    return true;
}


//===========================================================================================
//                                EXPLICIT TEMPLATE INSTANTIATIONS
//===========================================================================================


} // namespace ode::python