''' OnsetDetector example: find note onset times in an audio file by using
    the OnsetDetector with a DetectorBank and a detection threshold.
'''
import numpy as np
import scipy.io.wavfile
import detectorbank as db
 
# read audio from file
# the sample rate should be 48000 or 44100
sr, audio = scipy.io.wavfile.read('file.wav')

if audio.dtype == 'int16':
    audio = audio / 2**15

# array of frequencies (corresponding to 88 key piano)
f = np.array(list(440 * 2**(k/12) for k in range(-48, 40)))
# 5Hz bandwidth
bandwidth = np.zeros(len(f))
bandwidth.fill(5)
# make 2D array of frequencies and bandwidths
det_char = np.stack((f, bandwidth), axis=1)

# detectorbank  parameters
method = db.DetectorBank.runge_kutta
f_norm = db.DetectorBank.freq_unnormalized
a_norm = db.DetectorBank.amp_normalized
damping = 0.0001
gain = 25
 
# construct DetectorBank
det = db.DetectorBank(sr, audio.astype(np.float32), 4, det_char,  
                      method|f_norm|a_norm, damping, gain)

# create onsetdetector
od = db.OnsetDetector(det)

# create list to be filled with onsets
onsets = db.OnsetList()
# if another list ('vague') is provided, it will be filled with any instances
# where an onset was detected (threshold was exceeded), but an exact time could
# not be found
vague = db.OnsetList()

# fill the onsets list using a threshold of 0.2
od.findOnsets(onsets, 0.2, vague)
# if any detected onsets occur within 30ms of each other, they should be 
# regarded as simultaneous
od.sortSimultaneous(onsets)

# times and frequencies from the onsets list can be accessed thus: onsets[n].t
# and onsets[n].fv[0]

print('Found {} onsets'.format(len(onsets)))
# print onset times and frequencies
for n in range(len(onsets)):
    print('Onset at {:.3f}s, frequency: {:.3f}Hz'.format(onsets[n].t, 
          onsets[n].fv[0]), end='')
    for m in range(1, len(onsets[n].fv)):
        print(', {:.3f}Hz'.format(onsets[n].fv[m]), end='')
    print('\n', end='')
