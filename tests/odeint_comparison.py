from orbidyn import *

t, x, y, mu = symbols('t, x, y, mu')

#we declare the event when x=1 during the ode itegration
#If we pass it into the solver, every time it is encountered during an integration,
#it will be accurately determined using the provided tolerance.
ev1 = SymbolicPreciseEvent("X=1 event", x-1, event_tol=1e-20)
ev2 = SymbolicPeriodicEvent("Periodic Event", period=100)


ode_system = OdeSystem([y, mu*(1 - x**2)*y - x], t, [x, y], args=(mu,), events=[ev1, ev2])


def f_py(t, q, mu):
    x, y = q
    #we assume q = [x, y], so q[0] is x, and q[1] = y
    return [y, mu*(1 - x**2)*y - x]

def jac_py(t, q, mu):
    x, y = q
    return [[0, 1], [-2*mu*x*y - 1, mu*(1-x**2)]]

def event_py(t, q, mu):
    return q[0] - 1 #x-1 event -> when x-1 == 0, the event is triggered

ev1_py = PreciseEvent("X=1 event", event_py, event_tol=1e-20)
ev2_py = PeriodicEvent("Periodic Event", period=100)



#first let's define the fully low level ode object, where all internal functions are compiled to low level code
ode_lowlevel = ode_system.get(t0=0, q0=[1, 2], rtol=1e-13, atol=1e-13, args=(500,), method="BDF")
print("Only the first .get() call takes longer in order to compile all symbolic expressions")


'''
Alternatively, the user can retrieve the callable object that wraps the compiled function, and pass it inside
LowLevelODE. This is true for the optional jacobian, and the event objects too. The LowLevelODE internally
determines which objects are pure compiled functions, and retrieves the hidden C-style pointer to use that instead
of the python wrapper. As a result, there is zero performance loss when comparing to a C++ script that solves the same ODE
using the same C++ headers that have been precompiled for this python library.
'''

events_compiled = ode_system.true_compiled_events()
rhs_compiled = ode_system.lowlevel_odefunc('double')
jac_compiled = ode_system.lowlevel_jac('double') #jacobian matrix function

ode_test = LowLevelODE(rhs_compiled, t0=0, q0=[1, 2], jac=jac_compiled, rtol=1e-13, atol=1e-13, args=(500,), events = events_compiled, method="BDF")

#now let's define the ode object with pure python functions
ode_pythonic = LowLevelODE(f_py, jac=jac_py, t0=0, q0=[1, 2], rtol=1e-13, atol=1e-13, args=(500,), events=[ev1_py, ev2_py], method="BDF")


result_lowlevel = ode_lowlevel.integrate(400)
# result_test = ode_test.integrate(400)


result_pythonic = ode_pythonic.integrate(400)


from scipy.integrate import solve_ivp
import time

ti = time.time()
result_scipy = solve_ivp(f_py, (0, 400), [1, 2], jac=jac_py, method="BDF", rtol=1e-13, atol=1e-13, args=(500,))
tf = time.time()



print("Scipy implementation                  :", tf-ti, " s integration time")
print("LowLevelODE with pure python functions:", result_pythonic.runtime, " s integration time")
print("Fully compiled ODE                    :", result_lowlevel.runtime, " s integration time")
# print("Fully compiled test ODE               :", result_test.runtime, " s integration time")
print("As expected, the last two cases have the same performanace")
print("Speedup                               : x", (tf-ti)/result_lowlevel.runtime)






import matplotlib.pyplot as plt

class Orbit(LowLevelODE):

    @property
    def x(self):
        return self.q[:, 0]
    
    @property
    def y(self):
        return self.q[:, 1]
    
    def plot_x(self):
        fig, ax = plt.subplots()
        ax.plot(self.t, self.x)
        ax.set_xlabel("t")
        ax.set_ylabel("x")
        return fig, ax
    
    def plot_y(self):
        fig, ax = plt.subplots()
        ax.plot(self.t, self.y)
        ax.set_xlabel("t")
        ax.set_ylabel("y")
        return fig, ax
    
    def plot(self):
        fig, ax = plt.subplots()
        ax.plot(self.x, self.y)
        ax.set_xlabel("x")
        ax.set_ylabel("y")
        return fig, ax
    

orbit = Orbit(ode_lowlevel) #now ode_lowlevel can even be deleted, orbit created a perfect copy out of it
#to test this, see what happens before and after we advance the orbit.



print("Before integrating:")
print("     ode_lowlevel is at t =", ode_lowlevel.t[-1])
print("     orbit is at t =", orbit.t[-1])
orbit.integrate(10000)
print("After integrating:")
print("     ode_lowlevel is still at t =", ode_lowlevel.t[-1])
print("     orbit is at t =", orbit.t[-1])
print("So 'orbit' is completely independent of 'ode_lowlevel', even if it was passed in its constructor. A perfect copy was created.")



orbit.plot()
orbit.plot_x()


t_event, q_event = orbit.event_data("X=1 event")
q_event[:, 0] #this is the "x" variable at each event instance. As expected, this returns an array filled with approximately x=1