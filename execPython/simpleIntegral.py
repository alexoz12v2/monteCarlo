import math
import numpy as np
import numpy.typing as npt
import matplotlib.pyplot as plt
import sys
from numpy.random import default_rng
from typing import Generator, Annotated, Callable
from annotated_types import Gt

_rng : Generator = default_rng()

def integrand(x : float):
    arg : float = math.pi*math.sqrt(x*0.1)
    return (math.sin(arg)/arg)*math.e**arg;

# suppone il calcolo dell'integrale definito e' nell'intervallo [0,10]
def trueIntegralValue():
    return (10*math.e**math.pi+10)/(math.pi**2)

def monteCarloIntegration(integrandFunction : Callable[[float], float], domainSize : float, nSamples : Annotated[int, Gt(0)]):
    sum : float = 0.;
    for i in np.arange(0, nSamples):
        u : float = _rng.uniform()
        x_i = 10. * u + sys.float_info.epsilon * (u == 0)
        sum += integrandFunction(x_i)
    return domainSize * sum / nSamples

def main():
    maxSamples : Annotated[int, Gt(0)] = 1000
    trueRes : float = trueIntegralValue()
    results : npt.ArrayLike = np.zeros(maxSamples)
    xAxis : npt.ArrayLike = np.arange(1,maxSamples+1)
    for nSamples in np.arange(1,maxSamples+1):
        res : float = monteCarloIntegration(integrand, 10, nSamples)
        results[nSamples-1] = res

    errors = np.fromiter(map(lambda x: x-trueRes, results), dtype=np.dtype(float))

    fig, ax = plt.subplots()
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_title("Integrazione monte carlo errore")
    ax.plot(xAxis, errors, label="Errore")
    ax.legend()
    plt.show()

    for i in np.arange(0, maxSamples):
        print(f"({xAxis[i]}, {errors[i]}) ")

    return

main()
