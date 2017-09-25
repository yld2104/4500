#!/usr/bin/python
import sys
from arbwriter import writelp
from mysolver import lpsolver
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

if len(sys.argv) != 3:
    sys.exit("usage: arbtest.py datafilename lpfilename\n")

#now open and read data file
try:
    datafile = open(sys.argv[1], 'r') # opens the data file
except IOError:
    sys.exit("Cannot open file %s\n" % sys.argv[1])


lines = datafile.readlines();
datafile.close()

#print lines[0]
firstline = lines[0].split()
#print "first line is", firstline

numsec = int(firstline[1])
numscen = int(firstline[3])
r = float(firstline[5])
# print "\n"
# print "number of securities:", numsec,"number of scenarios", numscen,"r",r
# print "\n"

#allocate prices as one-dim array
p = [0]*(1 + numsec)*(1 + numscen)
k = 0
# line k+1 has scenario k (0 = today)
while k <= numscen:
    thisline = lines[k + 1].split()
#    print "line number", k+1,"is", thisline
    # should check that the line contains numsec + 1 words
    j = 1
#    print "scenario", k,":"
    p[k*(1 + numsec)] = 1 + r*(k != 0)
    while j <= numsec:
        value = float(thisline[j])
        p[k*(1 + numsec) + j] = value
#        print " sec ", j, " -> ", p[k*(1 + numsec) + j]
        j += 1
    k += 1

#now write LP file, now done in a separate function (should read data this way, as well)

lpwritecode = writelp(sys.argv[2], p, numsec, numscen)

# print "wrote LP to file", sys.argv[2],"with code", lpwritecode

#now solve lp
weights = lpsolver(sys.argv[2], "test.log")
if not isinstance(weights, int):
    weights = pd.DataFrame(weights)
    df_price = pd.DataFrame(np.asarray(p).reshape([numscen +1 , numsec+1]))

    n_sim = 100
    trials = np.zeros(n_sim)
    for sim in range(n_sim):
        perturb = pd.DataFrame(np.random.uniform(0.95,1.05, (numscen , numsec)))
        perturb.index = [i+1 for i in perturb.index]
        perturb.columns = [i+1 for i in perturb.columns]
        #print df_price[df_price.columns[1:]]
        #print df_price[df_price.columns[1:]].iloc[df_price.index[1:]]
        temp = df_price[df_price.columns[1:]].iloc[df_price.index[1:]]
        perturbed_price = temp * perturb
        perturbed_price = pd.concat([df_price[[df_price.columns[0]]].iloc[1:],perturbed_price], axis = 1)
        objectives = pd.DataFrame(np.dot(perturbed_price.values, weights.values))
        score = objectives <= 0
        trials[sim] = int(score.sum().values)
    trials = pd.DataFrame(trials).astype(int)
    # trials.plot(kind = 'hist', title = 'Total Score for ' + str(n_sim) + ' Simulations',  )
    n_bin = len(trials[0].unique())
    trials.hist(bins = n_bin)
    plt.title('Total Score for ' + str(n_sim) + ' Simulations')
    plt.show()
# print "solved LP at", sys.argv[2],"with code", lpsolvecode
