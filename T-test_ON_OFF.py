import numpy as np
import pandas as pd
from scipy import stats
import matplotlib.pyplot as plt
import sys

fname = sys.argv[1]

# Read the CSV file
data = pd.read_csv(fname, names=['j', 'state', 'value'], skipinitialspace=True)


# Separate on and off states and convert to float
on_values = data[data['state'] == 'on']['value'].astype(float).values
off_values = data[data['state'] == 'off']['value'].astype(float).values



# Perform independent samples t-test
t_stat, p_value = stats.ttest_ind(on_values, off_values)

# Display results
print('Filename: ', fname)
print(f"On state:  n={len(on_values)}, mean={np.mean(on_values):.2f}, std={np.std(on_values, ddof=1):.2f}")
print(f"Off state: n={len(off_values)}, mean={np.mean(off_values):.2f}, std={np.std(off_values, ddof=1):.2f}")
print(f"\nt-statistic: {t_stat:.4f}")
data_mean = 0.5*(np.mean(on_values)+np.mean(off_values))
eff_size = np.abs(np.mean(on_values)-np.mean(off_values))
eff_pct = eff_size/data_mean
print(f"p-value: {p_value:.4f}   Effect size: {eff_size:.4f} ({100*eff_pct:.1}%)")
print(f"\nResult: {'Reject'         if p_value < 0.05 else 'Fail to reject'} null hypothesis at α=0.05")
print(f"\nResult is: {'Significant' if p_value < 0.05 else 'Insignificant'}  at α=0.05")

# set data histogram plotting limits.   Constant limits facilitate graphical comparison
#
ndata_mean = 2440  # nominal values to standardize plot
neff_size  = 40     #   "       "
pltctr = ndata_mean
minplot = ndata_mean-2*neff_size
maxplot = ndata_mean+2*neff_size

nabove = 0
nbelow = 0
for i,ofv in enumerate(off_values):
    if ofv > maxplot:
        nabove+=1
    if ofv < minplot:
        nbelow+=1
    if on_values[i] > maxplot:
        nabove +=1
    if on_values[i] < minplot:
        nbelow +=1

print(f'Plotting Range: {minplot}--{maxplot}')
print(f'Plot Outlier Report:  {nabove} above, {nbelow} below.')

# Create histogram
plt.figure(figsize=(10, 6))
bins = np.linspace(minplot,maxplot,100)
plt.hist(off_values, bins=bins, alpha=0.5, label='Off', color='blue')
plt.hist(on_values, bins=bins, alpha=0.5, label='On', color='red')
plt.xlabel('Value')
plt.ylabel('Frequency')
plt.title(f'Distribution of On vs Off Values: {fname}')
plt.legend()
plt.grid(True, alpha=0.3)
plt.show()
