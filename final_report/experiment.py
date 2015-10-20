import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

data = pd.read_csv('../log.csv',sep=', ',engine='python')

print data.head()
wtf = np.array(data[['cycle','total reward']])
print wtf
