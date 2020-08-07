import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import sys

file = open(sys.argv[1], "r")

lines = file.readlines()

if sys.argv[1] == "results":
    throughput = []
    latency = []
    avelatency = []
    x = []
    for line in lines:
        list = line.split()
        x.append(float(list[0]))
        throughput.append(float(list[1]))
        latency.append(float(list[2]))
        avelatency.append(float(list[3]))
    x = np.array(x)        
    throughput = np.array(throughput)
    latency = np.array(latency)
    avelatency = np.array(avelatency)
    line1 = pd.Series(data = throughput, index = x)
    line2 = pd.Series(data = latency, index = x)
    line3 = pd.Series(data = avelatency, index = x)
    line1.plot(label="throughput", legend= True)
    plt.show()
    line2.plot(label = "latency", legend= True)
    line3.plot(label = "max latency", legend= True)
    plt.show()
    exit()
            

total = 0
counter = 0
num_errors = 0
dict = {}
array = []

for line in lines:
    line = line.strip()
    counter += 1
    if(float(line) == -1):
        num_errors += 1
        continue
    total += float(line)
    num = round(float(line), 3)
    if num in dict:
        dict[num] += 1
    else:
        dict[num] = 1
    array.append(num)
    
print(num_errors, "errors in", counter, "total requests")
print("average time taken:", total/counter)
print("max value:", max(array))


nparr = np.array(array)
plt.hist(nparr, bins=100)
plt.gca().set(title='Frequency Histogram', ylabel='Frequency');
plt.show()

