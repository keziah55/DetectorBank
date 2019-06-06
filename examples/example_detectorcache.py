''' DetectorCache example: create a DetectorBank and use a DetectorCache to 
    access abs(z) values on demand.
'''
import numpy as np
import scipy.io.wavfile
from detectorbank import DetectorBank, Producer, DetectorCache
 
# read audio from file
# the sample rate should be 48000 or 44100
sr, audio = scipy.io.wavfile.read('file.wav')

if audio.dtype == 'int16':
    audio = audio / 2**15

# array of frequencies (Hertz)
f = np.array(list(440 * 2 ** (k/12) for k in range(-5,5)))
bandwidth = np.zeros(len(f))
# change the bandwidth of the last detector
bandwidth[2] = 7
# make 2D array of frequencies and bandwidths
det_char = np.array(list(zip(f, bandwidth)))
         
# detectorbank  parameters
method = DetectorBank.runge_kutta
f_norm = DetectorBank.freq_unnormalized
a_norm = DetectorBank.amp_normalized
damping = 0.0001
gain = 25
 
# construct DetectorBank
det = DetectorBank(sr, audio.astype(np.float32), 0, det_char,
                   method|f_norm|a_norm, damping, gain)

# make a producer
p = Producer(det)

# 2 segments, each containing sr (48000) abs(z) values
cache = DetectorCache(p, 2, sr)

# get the first abs(z) value from channel five
result = cache[5,0]
print(result)

# get the 100000th value from channel seven
# this causes a new segment to be generated (100000 > 2*sr)
result = cache[7,100000]
print(result)

# attempting to access a value from the (now discarded) first segment will 
# cause the following exception to be thrown:
# IndexError: SlidingBuffer: Indexed item no longer available (underflow)
try:
    result = cache[3,1000]
except IndexError as err:
    print('Attempting to access a value from a discarded segment throws an exception:')
    print('IndexError: {}'.format(err))
