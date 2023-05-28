
# Esempio: Applicazione dell'algoritmo di Metropolis-Hastings per campionare una distribuzione gaussiana standard. 
# tale algoritmo ci permette di campionare samples con una data distrobuzione conoscendo soltanto una funzione simile 
# alla densita' di distribuzione target, non necessariamente normalizzata (nel calcolo della probabilita' di accettazione
# si divide e dunque si semplifica il fattore di normalizzazione).
# Scelgo come probabilita' di proposta p una gaussiana centrata in x_i con deviazione standard 0.05. In quanto tale 
# PDF simmetrica, q(x_i|x*) == q(x*|x_i) (cioe' il cammino casuale non dipende dalla direzione ma solo dalla distanza)
# cio vuol dire che le due probabilita' di transizione nel calcolo della probabilita' di accettazione si semplificano

# initialise x_0
# for i = 0 to N-1
#   sample u ~ U(0,1)
#   sample x* ~ q(x*|x_i)
#   if u < A(x_i, x*) = min{1 (p(x*)q(x_i|x*))/(p(x_i)q(x*|x_i))}
#       x_{i+1} = x*
#   else
#       x_{i+1} = x_i

import math
import matplotlib.pyplot as plt
import numpy as np
import numpy.typing as npt
from typing import Generator, Annotated
from annotated_types import Gt
from numpy.random import default_rng
from matplotlib.widgets import Slider, Button

def normPdf(value : float, mean : float = 0.0, stddev : float = 1.0):
    rSqrt2Pi : float = 0.39894228
    rStddev : float = 1 / stddev
    return rSqrt2Pi * rStddev * math.exp(-0.5*((value - mean)* rStddev)**2)

_rng : Generator = default_rng()

def metropolisHastings(samples : npt.ArrayLike, N : Annotated[int, Gt(0)]):
    for i in np.arange(N-1):
        u : float = _rng.uniform()
        proposedSample : float = _rng.normal(samples[i], 0.05)

        acceptance : float = np.clip(min(1, normPdf(proposedSample) / normPdf(samples[i])), 0., 1.)
        if u < acceptance:
            samples[i+1] = proposedSample
        else:
            samples[i+1] = samples[i]

    return
 
N : int = 100 
_samples : npt.ArrayLike = np.zeros(N)
metropolisHastings(_samples, N)

_samplesPdfs : npt.ArrayLike = np.fromiter(map(lambda x: np.clip(normPdf(x), 0., 1.), _samples), dtype=np.dtype(float))
_referenceArray : npt.ArrayLike = np.linspace(-3.5,+3.5, N)


fig, ax = plt.subplots()
ax.set_xlabel("x")
ax.set_ylabel("y")
ax.set_title("Comparazione Metropolis-Hastings e Reference")
_samplesLine, = ax.plot(_samples, _samplesPdfs, "o", label="Metropolis-Hastings")
ax.plot(_referenceArray, np.fromiter(map(lambda x: np.clip(normPdf(x), 0., 1.), _referenceArray), dtype=np.dtype(float)), label="PDF Normale Standard")
ax.legend()

# make room for the slider
fig.subplots_adjust(bottom = 0.25)

# slider which controls the number of samples taken
axSlider = fig.add_axes([0.23, 0.1, 0.65, 0.03])
samplesSlider = Slider(
    ax=axSlider,
    label="Numero Samples",
    valmin=10,
    valmax=250000,
    valinit=N
)

# callback per quando lo slider e' cambiato
def update(val):
    val : Annotated[int, Gt(0)] = math.trunc(val)
    _samples = np.zeros(val)
    metropolisHastings(_samples, val)
    _samplesPdfs = np.fromiter(map(lambda x: np.clip(normPdf(x), 0., 1.), _samples), dtype=np.dtype(float))
    _samplesLine.set_data(_samples, _samplesPdfs)
    fig.canvas.draw_idle()

samplesSlider.on_changed(update)

plt.show()
