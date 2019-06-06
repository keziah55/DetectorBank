import numpy as np

import sys
sys.stdout = open('pitches.inc', 'w')

print ("""
/* Frequency table for 12EDO, 88-key instrument.
   Starting from A0. */

const int DetectorBank::EDO12_pf_size = 88;
const parameter_t DetectorBank::EDO12_pf[] = {""")

def getB(fd):
    
    m = (np.log(532.883701808) - np.log(0.160401176805)) / np.log(15)
    
    logb = m * np.log(abs(fd)) + np.log(0.160401176805)
    
    return -np.exp(logb)

semitone = 2**(1/12)

f = np.array(list(440*2**(x/12) for x in range(-48,40)))
fd = np.zeros(len(f))
for n in range(len(f)):
    fd[n] = (abs(f[n]-f[n]*2**(-1/24)) + abs(f[n]-f[n]*2**(1/24)) ) / 2

b = getB(fd)

freqs = ",\n".join(["\t{0:.4f}".format(440 * semitone**x)
                   for x in range(-48,40)])
    
bArray = ",\n".join(["\t{0:.4f}".format(l) for l in b])

print(freqs + "\n};\n")


#print ("""
#const int DetectorBank::b_size = 88;
#const parameter_t DetectorBank::b[] = {""")

#print(bArray + "\n};\n")
