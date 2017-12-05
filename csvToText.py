import csv
import sys
import numpy as np

if len(sys.argv) != 2:
    sys.exit("usage: csvToText.py datafilename\n")
     

#open and read data file
try:
    csvfile = open(sys.argv[1],'rb')
    portreader = csv.reader(csvfile,skipinitialspace = True) # read the csv file and return a reader object
except IOError:
    sys.exit("Cannot open file %s\n" % sys.argv[1])

alist = []
alist_2 = []
day_length = 0
count = 0
for row in portreader:
    if not("russel" in sys.argv[1]):
        del row[-1]
    
    day_length = len(row)
    if (count == 0) and ("russel" in sys.argv[1]):
        print("not appending first row\n")
        count = count + 1
        continue
    if ("russel" in sys.argv[1]):
        alist.append(row[1:253])
        alist_2.append(row[253:])
    else:
        alist.append(row)
    count = count + 1
if "russel" in sys.argv[1]:
    count = count - 1
    day_length = 252

port_arr = np.array(alist).reshape(count, day_length)
port_arr_2 = np.array(alist_2).reshape(count, day_length)

N = port_arr.shape[0]
K = port_arr.shape[1]


datafilename = "dat" + sys.argv[1][:-4] + ".txt"
fp = open(datafilename, 'wb')
fp.write("N " + str(N) + "\n")
fp.write("K " + str(K) + "\n")

for i in range(N):
    for j in range(K):
        fp.write(str(port_arr[i,j]) + " ")
    fp.write("\n")

fp.close()

if "russel" in sys.argv[1]:
    datafilename = "dat" + sys.argv[1][:-4] + "-2.txt"
    fp = open(datafilename, 'wb')
    fp.write("N " + str(N) + "\n")
    fp.write("K " + str(K) + "\n")

    for i in range(N):
        for j in range(K):
            fp.write(str(port_arr_2[i,j]) + " ")
        fp.write("\n")

    fp.close()