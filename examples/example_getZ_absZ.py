''' Simple example to read in a file, create a DetectorBank with two octaves
    of detectors and plot the abs(z) values.
'''
import numpy as np
import scipy.io.wavfile
from detectorbank import DetectorBank
import matplotlib.pyplot as plt
 
# read audio from file
# the sample rate should be 48000 or 44100
sr, audio = scipy.io.wavfile.read('file.wav')

if audio.dtype == 'int16':
    audio = audio / 2**15

# array of frequencies (two octaves around A4)
f = np.array(list(440 * 2**(k/12) for k in range(-12, 13)))
# minimum bandwidth detectors
bandwidth = np.zeros(len(f))
# make 2D array of frequencies and bandwidths
det_char = np.array(list(zip(f, bandwidth)))

# detectorbank  parameters
method = DetectorBank.runge_kutta
f_norm = DetectorBank.freq_unnormalized
a_norm = DetectorBank.amp_normalized
damping = 0.0001
gain = 25
 
# create DetectorBank object with sample rate sr, 32-bit float input audio,  
# 4 threads, detector characteristics det_char, using Runge-Kutta, without
# frequency normalisation and with amplitude normalisation, damping = 0.0001
# and with a gain of 25
det = DetectorBank(sr, audio.astype(np.float32), 4, det_char, 
                   method|f_norm|a_norm, damping, gain)

# create empty output array
z = np.zeros((len(f),len(audio)), dtype=np.complex128)  
# fill z with detector output
det.getZ(z)

# fill new array r with the absolute values of the z array
r = np.zeros(z.shape)
m = det.absZ(r, z)
print('Max: {}'.format(m))

# plot the absZ values
t = np.linspace(0, r.shape[1]/sr, r.shape[1])
for k in range(r.shape[0]):
    line, = plt.plot(t,r[k])
plt.show()
