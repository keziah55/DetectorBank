    def test_003_notedetector_args(self):
        """Create a Note Detector with default arguments"""
                
        success = True
        nd = DB.NoteDetector(self.sr, self.buf, self.freqs, self.edo)
        expected = (
                    DB.NoteDetector.default_bandwidth,
                    DB.NoteDetector.default_features,
                    DB.NoteDetector.default_damping,
                    DB.NoteDetector.default_gain)
        result   = (nd.get_bandwidth(),
                    nd.get_features(),
                    nd.get_damping(),
                    nd.get_gain())
        comment  = 'default parameters are {}, expected {}'.format(result, expected)
        self.assertEqual(result, expected, msg=comment)
        
    def test_004_notedetector_args(self):
        """Supply one positional default argument"""
        new_bandwidth = 1
        nd = DB.NoteDetector(self.sr, self.buf, self.freqs, self.edo, new_bandwidth)
        expected = (new_bandwidth,
                    DB.NoteDetector.default_features,
                    DB.NoteDetector.default_damping,
                    DB.NoteDetector.default_gain)
        result   = (nd.get_bandwidth(),
                    nd.get_features(),
                    nd.get_damping(),
                    nd.get_gain())
        comment  = 'default parameters are {}, expected {}'.format(result, expected)
        self.assertEqual(result, expected, msg=comment)
        
    #BUG Using NDOptArgs, bandwidth and features get corrupted but gain is set ok.
    def test_005_notedetector_args(self):
        """Create a Note Detector with custom damping and default_gain: c++ method"""
        new_damping = 0.0003
        new_gain    = 35.0
        print("Creating note detector")
        nd = DB.NoteDetector(self.sr, self.buf, self.freqs, self.edo, 
                          DB.NDOptArgs().damping(new_damping).gain(new_gain))
        print("Done")
        expected = (DB.NoteDetector.default_bandwidth,
                    DB.NoteDetector.default_features,
                    new_damping, new_gain)
        result   = (nd.get_bandwidth(),
                    nd.get_features(),
                    nd.get_damping(),
                    nd.get_gain())
        self.assertEqual(result, expected,
                         msg='Failed to set features/damping/gain with NDOptArgs instance\n')

    def test_006_notedetector_args(self):
        """Create a Note Detector with custom damping and default_gain: kwargs method"""
        new_damping = 0.00025
        new_gain    = 75.0
        nd = DB.NoteDetector(self.sr, self.buf, self.freqs, self.edo,
                          damping=new_damping, gain=new_gain)
        expected = (DB.NoteDetector.default_bandwidth,
                    DB.NoteDetector.default_features,
                    new_damping, new_gain)
        result   = (nd.get_bandwidth(),
                    nd.get_features(),
                    nd.get_damping(),
                    nd.get_gain())
        self.assertEqual(result, expected,
                         msg='Failed to set features/damping/gain with kwargs\n')
    
    def test_007_notedetector_args(self):
        """Attept to construct a Note Detector mixing positional and keyword optional arguments"""
        with self.assertRaises(TypeError,
                               msg='Illegal mixture of positional and keyword opt args should thow a TypeError') :
            DB.NoteDetector(self.sr, self.buf, self.freqs, self.edo, 0, gain=37)
        with self.assertRaises(TypeError,
                               msg='Non-existant keyword argument should thow a TypeError') :
            DB.NoteDetector(self.sr, self.buf, self.freqs, self.edo, 0, gin=37)
    
    def test_008_amplitude_normalisation(self):
        """Make sure amplitude normalisation produces acceptable eccentricity (using f0=5Hz)"""
        from test_amp_norm import test_eccentricity
        
        # Supply the frequency at which the test is carried out
        eccentricity = test_eccentricity(5)
        # How round is round?
        correction_limit = 0.01
        
        msg = 'Original eccentricity {} corrected to {} (limit is {})'.format(
            eccentricity[DB.DetectorBank.amp_unnormalized],
            eccentricity[DB.DetectorBank.amp_normalized],
            correction_limit)
        
        self.assertLess(eccentricity[DB.DetectorBank.amp_normalized],
                        correction_limit,
                        msg)
        
    def tearDown(self):
        pass

