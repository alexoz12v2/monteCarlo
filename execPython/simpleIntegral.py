import math
import numpy as np
import numpy.typing as npt
import matplotlib.pyplot as plt
import sys
import time
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
    trueVal : float = trueIntegralValue()
    sum : float = 0.
    functionSamples : npt.ArrayLike = np.zeros(nSamples)
    for i in np.arange(0, nSamples):
        u : float = _rng.uniform()
        x_i = 10. * u + sys.float_info.epsilon * (u == 0)
        y_i = integrandFunction(x_i)
        sum += y_i
        functionSamples[i] = y_i

    mse = functionSamples.var() * domainSize * domainSize / nSamples
    return [domainSize * sum / nSamples, mse]

def monteCarloIntegrationBiased(integrandFunction : Callable[[float], float], domainSize : float, nSamples : Annotated[int, Gt(0)]):
    estimate : float = 0.
    for i in np.arange(0, nSamples):
        u : float = _rng.uniform()
        x_i = 10. * u + sys.float_info.epsilon * (u == 0)
        estimate = max(integrandFunction(x_i), estimate)
    return estimate / 2


def main():
    maxSamples : Annotated[int, Gt(0)] = 1000
    trueRes : float = trueIntegralValue()

    # estimated quantities
    results         : npt.ArrayLike = np.zeros(maxSamples)
    biasedResults   : npt.ArrayLike = np.zeros(maxSamples)
    times           : npt.ArrayLike = np.zeros(maxSamples) # millisecondi
    vars            : npt.ArrayLike = np.zeros(maxSamples)
    mses            : npt.ArrayLike = np.zeros(maxSamples) # in reality, variances, need to add bias^2

    xAxis : npt.ArrayLike = np.arange(1,maxSamples+1)
    for nSamples in np.arange(1,maxSamples+1):
        # set start time
        startTime = time.time() * 1000

        # calculations
        [res, mse]= monteCarloIntegration(integrand, 10, nSamples)
        resBiased = monteCarloIntegrationBiased(integrand, 10, nSamples)

        # set results
        results[nSamples-1] = res
        biasedResults[nSamples-1] = resBiased
        mses[nSamples-1] = mse
        times[nSamples-1] = time.time() * 1000 - startTime

    errors = np.fromiter(map(lambda x: x-trueRes, results), dtype=np.dtype(float))
    biasedErrors = np.fromiter(map(lambda x: x-trueRes, biasedResults), dtype=np.dtype(float))
    efficiencies = 1. / ( times * mses + sys.float_info.epsilon);

    fig, (ax1, ax2) = plt.subplots(1,2)
    ax1.set_xlabel("x")
    ax1.set_ylabel("y")
    ax1.set_title("Integrazione monte carlo errore")
    ax2.set_xlabel("x")
    ax2.set_ylabel("y")
    ax2.set_title("Integrazione monte carlo errore")
    ax1.plot(xAxis, mses, label="var")
    ax2.plot(xAxis, biasedErrors, label="Errore Biased Est.")
    ax1.legend()
    ax2.legend()
    plt.show()

    #for i in np.arange(0, maxSamples):
    #    print(f"({xAxis[i]}, {errors[i]}) ")

    # save error, biased_error, mse(var), efficiency (1/(var*elapsedTime))

    path = "../docs/assets/";
    filenames = ["chapter6_error_data.dat", "chapter6_biased_estimator_error_data.dat", "chapter6_mse_data.dat", "chapter6_efficiency.dat"]

    # Defining the custom format specifier
    format_specifier = lambda x, y: f"({x}, {y})\n"

    # Saving the pairs to files using np.savetxt with the custom format specifier
    np.savetxt(path + filenames[0], np.vectorize(format_specifier)(xAxis, errors), fmt='%s')
    np.savetxt(path + filenames[1], np.vectorize(format_specifier)(xAxis, biasedErrors), fmt='%s')
    np.savetxt(path + filenames[2], np.vectorize(format_specifier)(xAxis, mses), fmt='%s')
    np.savetxt(path + filenames[3], np.vectorize(format_specifier)(xAxis, efficiencies), fmt='%s')

    return

main()
