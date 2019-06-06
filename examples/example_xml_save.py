import numpy as np
from detectorbank import DetectorBank

# array of frequencies (corresponding to 88 key piano)
f = np.array(list(440 * 2**(k/12) for k in range(-48, 40)))
bandwidth = np.zeros(len(f))
det_char = np.array(list(zip(f, bandwidth)))

# detectorbank  parameters
sr = 48000
method = DetectorBank.runge_kutta
f_norm = DetectorBank.freq_unnormalized
a_norm = DetectorBank.amp_normalized
damping = 0.0001
gain = 25

# construct a DetectorBank, giving an empty array as input
det = DetectorBank(sr, np.zeros(1).astype(np.float32), 0, det_char, 
                   method|f_norm|a_norm, damping, gain)

# save the DetectorBank as '88 key piano, RK4, f un, a nrm'
det.saveProfile('88 key piano, RK4, f un, a nrm')
