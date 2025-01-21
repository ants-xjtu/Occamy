#!/usr/bin/python3

import matplotlib.pyplot as plt
import csv
import sys

if len(sys.argv) > 1:
    file = sys.argv[1]
else:
    print("No pcap file. Exited.")
    exit()

time = []
q1 = []
q2 = []
q3 = []
thres = []
axis = 0
count = 0

CSV_FILE = file + '.csv'
  
with open(CSV_FILE, 'r') as csvfile:
    lines = csv.reader(csvfile, delimiter=',')
    first = True
    for row in lines:
        if first:
            first = False
            continue
        if int(row[1]) > -1 :
            count += 1
            # if count > 4000000:
            #     break
            # print(row)
            axis = len(q1)
            time.append(int(row[0]) * 0.001)
            q1.append(int(row[1]))
            q2.append(int(row[2]))
            q3.append(int(row[3]))
            thres.append(int(row[4]))

# pkt = range(-axis, len(q1)-axis)
pkt = [i - time[axis] for i in time]
# print(time)
# print(time[axis])
# plt.axes(yscale = "log")
plt.plot(pkt, thres, color = 'g', label='threshold')
# plt.plot(pkt, [x/2 for x in thres], color = 'forestgreen')
plt.plot(pkt, q1, color = 'r', label='q1')
# plt.plot(pkt, q2, color = 'y', label='q2')
plt.plot(pkt, q3, color = 'b', label='q3')
# plt.scatter(pkt, thres, color = 'g', s=1)
plt.legend()
# plt.xticks(rotation = 25)
plt.xlabel('Time / μs')
plt.ylabel('Values / cell')
# plt.title('---', fontsize = 20)
  
plt.show()