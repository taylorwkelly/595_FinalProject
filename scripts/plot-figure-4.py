import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import statistics
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("path", type=str, help="The name of the outputfile produced by ns-3")
args = parser.parse_args()

file1 = open(args.path, 'r')
Lines = file1.readlines()
MAX_POSITION = 0

myDict = {}


#  Need to change these to the versions that we are going to use
# cara = "ns3::CaraWifiManager"
# aarf = "ns3::AaefWifiManager"
# minstrel = "ns3::MinstrelWifiManager"
myDict["ns3::MinstrelHtWifiManager"] = []
myDict["ns3::IdealWifiManager"] = []
myDict["ns3::ThompsonSamplingWifiManager"] = []

rates = []

manager = ""
position = 0
for line in Lines:
    line = line.strip()
    vals = line.split(",")
    if(vals[0] == "manager"):
        manager = vals[1]
    if(vals[0] == "position"):
        if(MAX_POSITION < int(vals[1])):
            MAX_POSITION = int(vals[1])
        position = vals[1]
    if(vals[0] == "summary"):
        myDict[manager].append((position,vals[6],rates))


lisToSort = myDict["ns3::MinstrelHtWifiManager"]
lisToSort.sort()
myDict["ns3::MinstrelHtWifiManager"] = lisToSort


lisToSort = myDict["ns3::IdealWifiManager"]
lisToSort.sort()
myDict["ns3::IdealWifiManager"] = lisToSort


lisToSort = myDict["ns3::ThompsonSamplingWifiManager"]
lisToSort.sort()
myDict["ns3::ThompsonSamplingWifiManager"] = lisToSort

print(myDict)


def extract_statistics(data, out_avg, out_std):
    tempList = []
    for i in range(MAX_POSITION+1):
        tempList.append([])
    for pos,thru,rate in data:
        tempList[int(pos)].append(float(thru))
    avg = 0
    for throughputs in tempList:
        t_sum = 0
        if (len(throughputs) == 0):
            continue
        if(len(throughputs) > 1):
            out_std.append(statistics.stdev(throughputs))
        nThru = len(throughputs)
        # print(throughputs)
        for thru in throughputs:
            t_sum+= float(thru)
        avg = t_sum/nThru
        out_avg.append(avg)


def bound(data,deviation,top_bound,lower_bound):
    for x in range(len(data)):
        top_bound.append(data[x]+deviation[x])
        lower_bound.append(data[x]-deviation[x])

mins_avg = []
mins_low = []
mins_high = []
mins_std = []

ideal_avg = []
ideal_low = []
ideal_high = []
ideal_std = []

thomp_avg = []
thomp_low = []
thomp_high = []
thomp_std = []


extract_statistics(myDict["ns3::MinstrelHtWifiManager"],mins_avg,mins_std)
bound(mins_avg,mins_std,mins_high,mins_low)


extract_statistics(myDict["ns3::IdealWifiManager"],ideal_avg,ideal_std)


extract_statistics(myDict["ns3::ThompsonSamplingWifiManager"],thomp_avg,thomp_std)
bound(thomp_avg,thomp_std,thomp_high,thomp_low)



positions = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99]

plt.plot(positions,mins_avg, label = "MinstrelHT")
plt.fill_between(positions, mins_high, mins_low, color="blue", alpha=0.1)
plt.plot(positions,ideal_avg, label = "Ideal", color="green")
plt.plot(positions,thomp_avg, label = "ThompsonSampling", color="yellow")
plt.fill_between(positions, thomp_high, thomp_low, color="yellow", alpha=0.1)
plt.xlabel("Distance (m)")
plt.ylabel("Throughput (Mb/s)")
plt.legend()
plt.grid()
plt.savefig("figure-4.png")

