import pytest
from pathlib import Path
import scipy.io.wavfile
import numpy as np

def _get_data_path():
    p = Path(__file__).parent.joinpath("data")
    return p

def read_audio(filename):
    sr, audio = scipy.io.wavfile.read(filename)
    n = 1
    if audio.dtype == np.int16:
        n = 2**15
    elif audio.dtype == np.int32:
        n = 2**31
    elif audio.dtype == np.uint8:
        n = 2**7
    audio = np.array(audio/n, dtype=np.dtype('float32'))
    return sr, audio

@pytest.fixture
def audio_results():
    p = _get_data_path()
    csv = p.joinpath('a4.csv')
    return csv

@pytest.fixture
def audio2_results():
    p = _get_data_path()
    csv = [p.joinpath(f) for f in ['0.csv', '1.csv']]
    return csv

@pytest.fixture
def audiofile():
    p = _get_data_path()
    return p.joinpath("a4.wav")

@pytest.fixture
def audiofile2():
    p = _get_data_path()
    return p.joinpath("dre48.wav")

@pytest.fixture
def audio(audiofile):
    return read_audio(audiofile)

@pytest.fixture
def audio2(audiofile2):
    return read_audio(audiofile2)

@pytest.fixture
def configfile():
    p = _get_data_path()
    return p.joinpath("hopfskipjump.xml")

@pytest.fixture
def atol():
    """ Absolute tolerance for np.isclose """
    return 0.01

