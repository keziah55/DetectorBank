#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Nov 16 11:35:20 2018

@author: keziah
"""


import numpy as np
from detectorbank import DetectorBank
import matplotlib.pyplot as plt


def getEllipseParams(x, y):
    """ Get the semimajor and semiminor axes, 'a' and 'b' respectively,
        of the ellipse defined by 'x' and 'y'
    """
    mx_x, mx_y = getMaxima(x,y)
    b, a = sorted([mx_x, mx_y])
    return a, b


def getMaxima(x, y):
    """ Get the semimajor and semiminor axes, 'a' and 'b' respectively,
        of the ellipse defined by 'x' and 'y'
    """
    mx_x = np.max(x)
    mx_y = np.max(y)
    return mx_x, mx_y

        
def plot_complex(f0, a_norm, do_plots):
    
    sr = 48000
    
    dur = 3
    t = np.linspace(0, 2*np.pi*f0*dur, sr*dur)
    audio = np.sin(t)
    audio = np.append(audio, np.zeros(sr))
    
    method = DetectorBank.runge_kutta
    f_norm = DetectorBank.freq_unnormalized
    
    d = 0.0001
    gain = 5
    f = np.array([f0])
    bandwidth = np.zeros(len(f))
    det_char = np.array(list(zip(f, bandwidth)))
    
    det = DetectorBank(sr, audio.astype(np.float32), 4, det_char, 
                       method|f_norm|a_norm, d, gain)
        
    
    z = np.zeros((len(f),len(audio)), dtype=np.complex128) 
    det.getZ(z)
    
    r = np.zeros(z.shape)
    det.absZ(r, z)
    
    # number of oscillations to plot
    nOsc = 5
    # signal may have been modulated by DetectorBank
    # detFreq is the frequency the detector is actually operating at, therefore
    # the frequency of the orbits
    detFreq = det.getW(0)/(2*np.pi)
    # number of samples in nOsc
    sOsc = int(nOsc/detFreq * sr)
    
    t0 = (dur*sr)-sOsc # int(0.0 * sr)
    t1 = dur*sr # int(nOsc/f[0] * sr)
    
    x = z[0].real[t0:t1]
    y = z[0].imag[t0:t1]
    
    c = ['darkmagenta']
    
    a, b = getEllipseParams(x, y)
    e = np.sqrt(a**2 - b**2) / a

    if do_plots:
        plt.plot(x, y, c[0])
        plt.grid()
        
        plt.title('{}Hz'.format(f0))
        plt.xlabel('real')
        plt.ylabel('imag')
            
        plt.show()
        plt.close()
    
    return e
    

def test_eccentricity(f0, do_plots = False):
    a_norms = {'Unnormalised' : DetectorBank.amp_unnormalized, 
               'Normalised' : DetectorBank.amp_normalized}
    
    normalisation_results = {}
    
    for k in a_norms.keys():
        a_norm = a_norms[k]
        e = plot_complex(f0, a_norm, do_plots)
        
        if do_plots:
            print('** {} **'.format(k))
            print('Eccentricity: {:g}\n'.format(e))
        
        normalisation_results[a_norm] = e
    
    return normalisation_results
        
            
if __name__ == '__main__':
    # Plot tests for 5Hz
    test_eccentricity(5, True)


    
