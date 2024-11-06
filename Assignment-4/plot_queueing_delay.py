import matplotlib.pyplot as plt
import os

folder = "Q3"

file_name = "tcp-example.tr"

seq_times = {}

def get_seq_num(line):
    return int(line.split(" ")[36][4:])

times = []
weights = []

with open(os.path.join(folder, file_name)) as f:
    for line in f:
        line = line.strip()

        if "/NodeList/0/DeviceList/0/" not in line:
            continue

        if line.startswith("+"):
            seq_times[get_seq_num(line)] = float(line.split(" ")[1])

        if line.startswith("-"):
            t = float(line.split(" ")[1])
            w = t - seq_times[get_seq_num(line)]
            times.append(t)
            weights.append(w)

plt.plot(times, weights)

max_weight = max(weights)
print(max_weight, times[weights.index(max_weight)])

plt.xlabel("Time (s)")
plt.ylabel("Queueing Delay (s)")
plt.title("Queueing Delay over time")
plt.show()
