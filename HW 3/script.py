#!/usr/bin/python

import sys
import pandas as pd
import numpy as np

def mean_cov(df, K, n):
    if K > 1000 or K < 0 or n > 250 or n < 0:
        return "Invalid arguments"
    if K > 1000 or K < 0 or n > 250 or n < 0:
        return "Invalid arguments"
    returns = df[df.columns[:K]].iloc[:n]/df[df.columns[:K]].iloc[:n].shift(1)-1
    mean = returns.mean()
    lower = pd.Series(np.zeros(K))
    upper = pd.Series(np.ones(K)*max(0.01,2.0/K))
    mean_output = pd.concat([lower, upper, mean], axis = 1)
    return mean_output, returns.cov()

if len(sys.argv) != 4:
    sys.exit("usage: arbtest.py datafilename K n\n")
sys.stdout=open("output.txt","w")
filename = sys.argv[1]

try:
    df = pd.read_csv(filename, header=None, usecols=range(250)).transpose()
except IOError:
    sys.exit("Cannot open file %s\n" % sys.argv[1])

K, n = (int(x) for x in sys.argv[2:])
mean_output, cov_output = mean_cov(df, K, n)
print("n" + " " + str(K) + '\n')
print("j_lower_upper_mu" + '\n')
print(mean_output.to_string(header=None))
print('\n' + "lambda" + " " + str(10) + '\n')
print("covariance" + '\n')
print(cov_output.to_string(header=None, index=False))
print('\n' + "END")
sys.stdout.close()
