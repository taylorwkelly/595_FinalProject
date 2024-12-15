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
cara = "ns3::CaraWifiManager"
aarf = "ns3::AarfWifiManager"
minstrel = "ns3::MinstrelWifiManager"

myDict[minstrel] = []
myDict[aarf] = []
myDict[cara] = []

size = 1000
standards = ["802.11a" , "802.11g"]
# pathLoss = [1, 2, 3]

# Position: What the distance parameter was set at
# Order of parameters from 4-c summary:
# summary, Sent Packets, Sent Bytes, Transmit Throughput, received packets, received bytes, 
# RECEIVER THROUGHPUT, Data Rate, Average received power, packet size

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
        myDict[manager].append((position,(int(vals[1])-int(vals[4])),vals[9],vals[10]))


lisToSort = myDict[minstrel]
lisToSort.sort()
myDict[minstrel] = lisToSort


lisToSort = myDict[aarf]
lisToSort.sort()
myDict[aarf] = lisToSort


lisToSort = myDict[cara]
lisToSort.sort()
myDict[cara] = lisToSort

# print(myDict[minstrel])


# Can modify this to extract more of the stats for a given program
def extract_statistics(data, out_avg, out_std):
    tempList = []
    for i in range(MAX_POSITION+1):
        tempList.append([])
    for pos, packloss, pack_size, stand in data:
        # print(pos, thru, pack_size)
        tempList[int(pos)].append(float(packloss))
    # print(tempList)
    avg = 0
    for packLosses in tempList:
        t_sum = 0
        if (len(packLosses) == 0):
            continue
        if(len(packLosses) > 1):
            out_std.append(statistics.stdev(packLosses))
        nThru = len(packLosses)
        # print(throughputs)
        for thru in packLosses:
            t_sum+= float(thru)
        avg = t_sum/nThru
        out_avg.append(avg)


def bound(data,deviation,top_bound,lower_bound):
    print(len(data))
    print(len(deviation))
    for x in range(len(data)):
        top_bound.append(data[x]+deviation[x])
        lower_bound.append(data[x]-deviation[x])




positions = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99]


for stand in standards:
  mins_avg = []
  mins_low = []
  mins_high = []
  mins_std = []

  aarf_avg = []
  aarf_low = []
  aarf_high = []
  aarf_std = []

  cara_avg = []
  cara_low = []
  cara_high = []
  cara_std = []
  extract_statistics(myDict[minstrel],mins_avg,mins_std)
  bound(mins_avg,mins_std,mins_high,mins_low)

  extract_statistics(myDict[aarf],aarf_avg,aarf_std)
  bound(aarf_avg,aarf_std,aarf_high,aarf_low)


  extract_statistics(myDict[cara],cara_avg,cara_std)
  bound(cara_avg,cara_std,cara_high,cara_low)
  # pathType = ""
  # if path == 1:
  #     pathType = "NakagamiPropagationLossModel"
  # if path == 2:
  #     pathType = "FriisPropagationLossModel"
  # else:
  #     pathType = "LogDistancePropagationLossModel"

  plt.figure()
  plt.plot(positions,mins_avg, label = "Minstrel")
  plt.fill_between(positions, mins_high, mins_low, color="blue", alpha=0.1)
  plt.plot(positions,aarf_avg, label = "AARF", color="green")
  plt.fill_between(positions, aarf_high, aarf_low, color="green", alpha=0.1)
  plt.plot(positions,cara_avg, label = "CARA", color="yellow")
  plt.fill_between(positions, cara_high, cara_low, color="yellow", alpha=0.1)
  plt.xlabel("Distance (m)")
  plt.ylabel("Packets Lost")
  plt.legend()
  plt.title(f"Distance vs Packet Loss")
  plt.grid()
  plt.tight_layout()
  plt.savefig(f"pack_loss_{stand}.png")







