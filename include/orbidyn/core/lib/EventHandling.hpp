#ifndef ORBIDYN_EVENTHANDLING_HPP
#define ORBIDYN_EVENTHANDLING_HPP


#include <odecraft/Core/Events.hpp>
#include "Tools.hpp"

namespace ode::python {

template<typename... T>
using variant_event_t = std::variant<pbox::Box<Event<T>>...>;

using event_generic_t = variant_event_t<ORBIDYN_SCALARS>;

class PyEvent : public DtypeDispatcher{

public:

    PyEvent(std::string name, py::object mask, bool delay_mask, const std::string& scalar_type, size_t Nsys, size_t Nargs);

    py::str                 name() const;

    bool                    mask_delayed() const;

    bool                    is_lowlevel() const;

    void                    check_sizes(size_t Nsys, size_t Nargs) const;

    virtual event_generic_t toEvent(const pyshape_t& shape, py::tuple args) const = 0;

    virtual ~PyEvent() = default;

protected:

    template<typename T>
    static auto             mask_compiled(py::tuple pyargs, raw_pyrhs_t<T> mask){
        std::vector<T> args = to_vector<T>(pyargs);
        return [=](T* out, const T& t, const T* q){
            mask(out, t, q, args.data());
        };
    }
    
    template<typename T>
    auto                    mask_pythonic(const pyshape_t& shape, py::tuple pyargs) const{
        return [=, mask=py_mask_](T* out, const T& t, const T* q){
            return rhs_pythonic<T>(out, t, q, shape, pyargs, mask);
        };
    }
    
    std::string name_;
    size_t nsys_;
    size_t nargs_;
    py::function py_mask_;
    RawPyRhs_t mask_{};
    bool delay_mask_;
    bool is_lowlevel_ = false;
    bool is_masked_ = false;
    bool requires_check_sizes_ = false;
};


class PyPrecEvent : public PyEvent {

public:

    PyPrecEvent(std::string name, py::object when, int dir, py::object mask, bool delay_mask, py::object event_tol, const std::string& scalar_type, size_t Nsys, size_t Nargs);

    DEFAULT_RULE_OF_FOUR(PyPrecEvent);

    py::object event_tol() const;;

    event_generic_t toEvent(const pyshape_t& shape, py::tuple args) const override;

protected:

    int dir_ = 0;
    RawPyObjFun_t obj_fun_{};
    py::function py_objfun_;
    py::object event_tol_;
};



class PyPerEvent : public PyEvent{

public:

    PyPerEvent(std::string name, py::object period, py::object mask, bool delay_mask, const std::string& scalar_type, size_t Nsys, size_t Nargs);

    DEFAULT_RULE_OF_FOUR(PyPerEvent);

    event_generic_t toEvent(const pyshape_t& shape, py::tuple pyargs) const override;

    py::object period() const;

private:
    py::object period_;

};

template<typename T>
EventList<T> to_Events(const py::iterable& events, const pyshape_t& shape, const py::iterable& args){
    // assumes that all event objects in `events` are of `T` scalar type
    if (events.is_none()){
        return {};
    }

    EventList<T> res;
    res.reserve(py::len(events));
    for (py::handle item : events){
        event_generic_t gen_event = item.cast<PyEvent&>().toEvent(shape, args);
        assert(std::holds_alternative<BoxedEvent<T>>(gen_event) && "Invalid event variant");
        // assert(g)
        BoxedEvent<T> event = std::move(std::get<BoxedEvent<T>>(gen_event));
        res.emplace_back(std::move(event));
    }
    return res;
}

bool all_are_lowlevel(const py::iterable& events);

} // namespace ode::python


#endif // ORBIDYN_EVENTHANDLING_HPP