from detectorbank import DetectorBank, Producer, DetectorCache
import numpy as np

def test_detectorbank(audio, audio_results, atol):
    method = DetectorBank.runge_kutta
    f_norm = DetectorBank.freq_unnormalized
    a_norm = DetectorBank.amp_unnormalized
    damping = 0.0001
    gain = 25
    
    f = np.array([440*2**(k/12) for k in range(-3,3)])
    bw = np.zeros(len(f))
    det_char = np.column_stack((f,bw))
    
    sr, audio = audio
    detbank = DetectorBank(sr, audio, 0, det_char, method|f_norm|a_norm, damping, gain)
    
    z = np.zeros((len(f),len(audio)), dtype=np.complex128)  
    detbank.getZ(z)

    result = np.zeros(z.shape)
    detbank.absZ(result, z)
    
    expected = np.loadtxt(audio_results)
    assert np.all(np.isclose(result, expected, atol=atol))
    
    
    
    