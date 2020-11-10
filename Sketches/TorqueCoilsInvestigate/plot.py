#%% Imports
from matplotlib import pyplot as plt
import numpy as np
import math

filename_1 = "run_4_pos.csv"
filename_2 = "run_4_neg.csv"

#%% Load data
data_1 = np.genfromtxt(filename_1, delimiter=",")
data_2 = np.genfromtxt(filename_2, delimiter=",")

# %%
fig, (ax1, ax2) = plt.subplots(1, 2, subplot_kw=dict(projection='polar'))

ax1.plot(data_1[:,1] / 255 * math.pi * 2, data_1[:,2], '.r')
ax1.plot(data_1[:,1] / 255 * math.pi * 2, data_1[:,3], '.b')

ax2.plot(data_2[:,1] / 255 * math.pi * 2, data_2[:,2], '.r')
ax2.plot(data_2[:,1] / 255 * math.pi * 2, data_2[:,3], '.b')

plt.show()