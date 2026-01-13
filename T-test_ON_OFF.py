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
print(f"p-value: {p_value:.4f}")
print(f"\nResult: {'Reject' if p_value < 0.05 else 'Fail to reject'} null hypothesis at α=0.05")

ofv1 = []
onv1 = []
for i,ofv in enumerate(off_values):
    if i>296:
        break
    if ofv > 2000:
        ofv1.append(ofv)
    if on_values[i] > 2000:
        onv1.append(on_values[i])


# Create histogram
plt.figure(figsize=(10, 6))
plt.hist(ofv1, bins=20, alpha=0.5, label='Off', color='blue')
plt.hist(onv1, bins=20, alpha=0.5, label='On', color='red')
plt.xlabel('Value')
plt.ylabel('Frequency')
plt.title('Distribution of On vs Off Values')
plt.legend()
plt.grid(True, alpha=0.3)
plt.show()
