''' FrequencyShifter example: create a sine tone at a given frequency, then
    modulate it and plot the power spectral density of both signals.
'''
import numpy as np
from detectorbank import FrequencyShifter
import matplotlib.pyplot as plt

sr = 48000
# original signal frquency
f0 = 10000
# amount by which to shift the signal
f_shift = -7000

# create sine tone
t = np.linspace(0, 2*np.pi*f0, sr)
signal = np.sin(t)
signal = signal.astype(np.float32)

# create empty array to be filled with the shifted signal
shifted = np.zeros(signal.shape, dtype=np.float32)
# make FrequencyShifter which uses FIR
fs = FrequencyShifter(signal, sr, FrequencyShifter.fir)
# shift the signal by the stated frequency
fs.shift(f_shift, shifted)

# the FrequencyShifter can be reused to modulate the same signal multiple times
#shifted2 = np.zeros(signal.shape, dtype=np.float32)
#fs.shift(5000, shifted2)

# plot power spectral density curves for the original and shifted signals
f, (ax1, ax2) = plt.subplots(2, sharex=True)
ax1.psd(signal, Fs=sr)
ax1.set_xlim(xmax=sr//2)
ax1.set_ylim(-120, -20)
ax1.set_xlabel('')
ax1.set_ylabel('dB/Hz')
ax2.psd(shifted, Fs=sr)
ax2.set_xlim(xmax=sr//2)
ax2.set_ylim(-120, -20)
ax2.set_ylabel('dB/Hz')
plt.show()