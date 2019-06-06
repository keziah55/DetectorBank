#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Dec  6 11:00:25 2016

@author: keziah
"""

import numpy as np
from detectorbank import DetectorBank
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator

def absZ_test(sr):
    """Feed a signal at given sample rate
    to the detectorbank and take the absolute value
    of the response.
    
    Return the result, and max error comparing
    the abzZ method results and numpy.abs() results."""
    f = np.array([200])
    t = np.linspace(0, f[0]*2*np.pi, sr)
    audio = np.sin(t)
    #audio = np.append(audio, np.zeros(sr))

    z = np.zeros((len(f),len(audio)), dtype=np.complex128)
    method = DetectorBank.runge_kutta
    f_norm = DetectorBank.freq_unnormalized
    a_norm = DetectorBank.amp_normalized
    d = 0.0001
    bandwidth = np.zeros(len(f))
    det_char = np.array(list(zip(f, bandwidth)))
    gain = 25
    det = DetectorBank(sr, audio.astype(np.float32), 4, det_char, 
                    method|f_norm|a_norm, d, gain)

    det.getZ(z)

    r = np.zeros(z.shape)
    _ = det.absZ(r, z)

    return r, np.amax(np.abs(r-np.abs(z)))

if __name__ == "__main__":
    sr = 48000
    t0, t1 = int(0*sr), int(0.2*sr)
    c = ['darkmagenta', 'lime']
    r,e = absZ_test(sr)
    #for k in range(len(z)):
    #    plt.plot(z[k].real[t0:t1], z[k].imag[t0:t1], c[k])
    #
    #ax = plt.gca()
    #majorLocator = MultipleLocator(0.2)
    #minorLocator = MultipleLocator(0.1)
    #ax.xaxis.set_major_locator(majorLocator)
    #ax.xaxis.set_minor_locator(minorLocator)
    #ax.yaxis.set_major_locator(majorLocator)
    #ax.yaxis.set_minor_locator(minorLocator)
    #ax.grid(True, 'both')
    #plt.xlabel('real(z)')
    #plt.ylabel('imag(z)')
    ##plt.savefig('100Hz_spiral_1.pdf', format='pdf')
    #plt.show()
    #plt.close()

    audioLen = r.shape[1]
    t = np.linspace(0, audioLen/sr, audioLen)
    for k in range(len(r)):
        plt.plot(t, r[k], c[k])
    #    print('Max in channel {}: {}'.format(k, max(r[k])))
    ax = plt.gca()
    ax.grid(True, 'both')
    plt.xlabel('time (s)')
    plt.ylabel('|z|', rotation='horizontal')
    ax.yaxis.labelpad = 10
    #plt.savefig('100Hz_line_1.pdf', format='pdf')
    plt.show()
    plt.close()
