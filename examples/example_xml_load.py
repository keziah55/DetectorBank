import numpy as np
import detectorbank as db
import scipy.io.wavfile

# [xml_load]
# load new audio file
sr, audio = scipy.io.wavfile.read('file.wav')

if audio.dtype == 'int16':
    audio = audio / 2**15
 
# construct a DetectorBank from the saved profile
det = db.DetectorBank('88 key piano, RK4, f un, a nrm', audio.astype(np.float32))
# [xml_load]