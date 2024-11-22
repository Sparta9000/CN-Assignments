import matplotlib.pyplot as plt
import os

folder = "Q1_test"

file_name = "tcp-example.cwnd"

with open(os.path.join(folder, file_name)) as f:
    lines = f.readlines()
    times = []
    start_cwnds = []
    end_cwnds = []
    for line in lines:
        time, start_cwnd, end_cwnd = line.split("\t")
        time = float(time.strip())
        start_cwnd = int(start_cwnd.strip())
        end_cwnd = int(end_cwnd.strip())

        times.append(time)
        start_cwnds.append(start_cwnd)
        end_cwnds.append(end_cwnd)

plt.plot(times, start_cwnds, label="Start CWND")
# plt.plot(times, end_cwnds, label="End CWND")

max_cwnd = max(start_cwnds)
print(max_cwnd, times[start_cwnds.index(max_cwnd)])

plt.xlabel("Time (s)")
plt.ylabel("CWND")
plt.title("CWND over time")
plt.show()
