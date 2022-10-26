import unittest
import tap
import os
import numpy as np

class DetectorbankTest(unittest.TestCase):

    def setUp(self):
        # Notedetector test variables
        self.buf   = np.zeros(1024, dtype=np.float32)
        self.sr    = 48000
        self.edo = 12
        self.freqs = np.array([261.626, 277.183, 293.665, 311.127,
                               329.628, 349.228, 369.994, 391.995,
                               415.305, 440.000, 466.164, 493.883, 523.251])

    # Now use assertTrue() assertFalse(), assertEqual() etc
    # to define test results. The comment argument is optional.
    # See https://docs.python.org/3/library/unittest.html
    
    def test_001_import(self):
        """Import detectorbank extension"""

        try:
            globals()['DB'] = __import__('detectorbank')
            success=True
            comment=''
        except:
            success=False
            comment="Couldn't load DetectorBank extension with\n\tPYTHONPATH=" + \
                os.environ['PYTHONPATH'] + '\n\tLD_LIBRARY_PATH=' + \
                os.environ['LD_LIBRARY_PATH']
        
        self.assertTrue(success, msg=comment)

    def test_002_absZ(self):
        """Check multi-threaded absZ works"""
        global Py_absZtest
        import Py_absZtest
        
        comment = ''
        _, err = Py_absZtest.absZ_test(48000)
        comment = 'multithreaded absZ error = {}'.format(err)
        
        self.assertAlmostEqual(err, 0, msg=comment)
    
# Notedetector tests have been moved into Py_notedetectortests.py to permit 
# breaking the notedetector out into a separate project

if __name__ == '__main__':
  tapRunner=tap.TAPTestRunner()
  tapRunner.set_stream(True)
  unittest.main(testRunner=tapRunner) # output='test-reports'
