from detectorbank import DetectorBank, Producer, DetectorCache
import numpy as np
import pytest

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
    
    
def test_detectorcache(audio2, atol):
    method = DetectorBank.runge_kutta
    f_norm = DetectorBank.freq_unnormalized
    a_norm = DetectorBank.amp_unnormalized
    damping = 0.0001
    gain = 25

    f = np.array([440*2**(k/12) for k in range(-12,12)])
    bw = np.zeros(len(f))
    det_char = np.column_stack((f,bw))

    sr, audio = audio2
    detbank = DetectorBank(sr, audio, 0, det_char, method|f_norm|a_norm, damping, gain)

    num_segs = 2
    seg_size = 5000
    producer = Producer(detbank)
    cache = DetectorCache(producer, num_segs, seg_size)

    # get the first abs(z) value from channel 7
    k = 7
    n = 0
    result = cache[k,n]
    expected_result = 0
    assert np.isclose(result, expected_result, atol=atol)


    # get segment 4, i.e. generate new segments and forget old
    k = 10
    n = (num_segs+2)*seg_size
    result = cache[k,n]
    expected_result = 0.65
    assert np.isclose(result, expected_result, atol=atol)

    # attempting to access a value from the (now discarded) first segment will
    # cause the following exception to be thrown:
    # IndexError: SlidingBuffer: Indexed item no longer available (underflow)
    with pytest.raises(IndexError):
        result = cache[3,seg_size//2]


