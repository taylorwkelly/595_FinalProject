import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import statistics
import pandas as pd
import numpy as np
import math
import sys

SMALL_SIZE = 12
MEDIUM_SIZE = 14
BIGGER_SIZE = 16

plt.rc('font', size=BIGGER_SIZE)          # controls default text sizes
plt.rc('axes', labelsize=MEDIUM_SIZE)    # fontsize of the x and y labels
plt.rc('xtick', labelsize=BIGGER_SIZE)    # fontsize of the tick labels
plt.rc('ytick', labelsize=BIGGER_SIZE)    # fontsize of the tick labels
plt.rc('legend', fontsize=BIGGER_SIZE)    # legend fontsize
plt.rc('figure', titlesize=SMALL_SIZE)  # fontsize of the figure title

def plot_convergenceD(dg, out, snr, mcs0, mcs1, mcs2, mcs3, mcs4, mcs5, mcs6, mcs7, mcs8, wifiManager):
    df = dg.copy()

    # Calculate average time to converge and standard deviation for each (test, manager) pair
    df_avg = df.groupby(['test', 'manager']).agg(avg_time_to_converge=('timetoconverge', 'mean'),
                                                 std_time_to_converge=('timetoconverge', 'std'))

    # Reshape the data to have one column per manager
    df_pivot = df_avg.reset_index().pivot(index='test', columns='manager', values='avg_time_to_converge')

    # Depict managers with different colors
    colors = dict([('MinstrelHt', 'blue'), ('ThompsonSampling', 'orange')])

    # Plot the lines with different colors for each manager
    fig, ax1 = plt.subplots()
    for i, manager in enumerate(df_pivot.columns):
        p1 = ax1.plot((df_pivot.index), df_pivot[manager], color=colors[wifiManager], label='Sample mean of 100 trials', clip_on=False)

    # # Set x ticks at all data points
    ax1.set_xticks(df_pivot.index[::5])
    xtick_labels = []

    # Set x tick labels every 3 ticks
    for i,x in enumerate(df_pivot.index[::5]):
        if(i%3 == 0):
            xtick_labels.append(str(round(x,1))+'dB')
        else:
            xtick_labels.append('')


    ax1.set_xticklabels(xtick_labels,fontsize=6)

    # Rotate x tick labels for better readability
    ax1.tick_params(axis='x', rotation=45)

    ax1.set_ylim(0,10)

    if(wifiManager == "MinstrelHt"):
        title = 'MinstrelHt convergence time vs. final SNR'
        ax1.set_ylabel('Time until last reported rate change (sec)', fontsize=12)
    else:
        title = 'ThompsonSampling convergence time vs. final SNR'
        ax1.set_ylabel('Convergence Time (sec)', fontsize=12)

    ax1.tick_params(axis='y', labelsize=12)
    ax2 = ax1.twinx ()
    ax2.set_ylabel('MPDU error rate', fontsize=12)
    ax2.set_ylim(0.0001, 1)
    ax2.tick_params(axis='y', labelsize=12)
    ax2.semilogy()
    p2 = ax2.plot(snr, mcs0, color='black', linestyle='dashed', alpha=0.5, label='PER curves for MCS 0 through 8')
    ax2.plot(snr, mcs1, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs2, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs3, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs4, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs5, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs6, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs7, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs8, color='black', linestyle='dashed', alpha=0.5)
    
    ax1.get_shared_x_axes().join(ax1, ax2)
    plots = p1 + p2
    labels = [l.get_label() for l in plots]
    ax2.legend(plots, labels, loc=1, fontsize=10)

    ax1.set_xlabel('Final SNR after a step decrease from initial 32 dB', fontsize=12)
    ax1.set_title(title, fontsize=14)
    ax1.spines['right'].set_visible(False)
    ax1.spines['top'].set_visible(False)
    ax1.spines['left'].set_linewidth(0.5)
    ax1.spines['bottom'].set_linewidth(0.5)
    ax2.grid(axis='both', which='both', alpha=0.4)
    ax1.grid(axis='x', which='both', alpha=0.4)
    plt.savefig(out,dpi=300,bbox_inches='tight')


def plot_convergenceU(dg, out, snr, mcs0, mcs1, mcs2, mcs3, mcs4, mcs5, mcs6, mcs7, mcs8, wifiManager):
    df = dg.copy()

    # Calculate average time to converge and standard deviation for each (test, manager) pair
    df_avg = df.groupby(['test', 'manager']).agg(avg_time_to_converge=('timetoconverge', 'mean'),
                                                 std_time_to_converge=('timetoconverge', 'std'))
    # Reshape the data to have one column per manager
    df_pivot = df_avg.reset_index().pivot(index='test', columns='manager', values='avg_time_to_converge')

    # Depict managers with different colors
    colors = dict([('MinstrelHt', 'blue'), ('ThompsonSampling', 'orange')])

    # Plot the lines with different colors for each manager
    fig, ax1 = plt.subplots()

    for i, manager in enumerate(df_pivot.columns):
        p1 = ax1.plot((df_pivot.index), df_pivot[manager], color=colors[wifiManager], label='Sample mean of 100 trials', clip_on=False)

    # # Set x ticks at all data points
    ax1.set_xticks(df_pivot.index[::5])
    xtick_labels = []

    # Set x tick labels every 5 ticks
    for i,x in enumerate(df_pivot.index[::5]):
        if(i%3 == 0):
            xtick_labels.append(str(round(x,1))+'dB')
        else:
            xtick_labels.append('')

    ax1.set_xticklabels(xtick_labels,fontsize=6)

    # Rotate x tick labels for better readability
    ax1.tick_params(axis='x', rotation=45)

    ax1.set_ylim(0,10)

    if(wifiManager == "MinstrelHt"):
        title = 'MinstrelHt convergence time vs. final SNR'
        ax1.set_ylabel('Time until last reported rate change (sec)', fontsize=12)
    else:
        title = 'ThompsonSampling convergence time vs. final SNR'
        ax1.set_ylabel('Convergence Time (sec)', fontsize=12)

    ax1.tick_params(axis='y', labelsize=12)
    ax2 = ax1.twinx ()
    ax2.set_ylabel('MPDU error rate', fontsize=12)
    ax2.set_ylim(0.0001, 1)
    ax2.tick_params(axis='y', labelsize=12)
    ax2.semilogy()
    p2 = ax2.plot(snr, mcs0, color='black', linestyle='dashed', alpha=0.5, label='PER curves for MCS 0 through 8')
    ax2.plot(snr, mcs1, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs2, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs3, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs4, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs5, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs6, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs7, color='black', linestyle='dashed', alpha=0.5)
    ax2.plot(snr, mcs8, color='black', linestyle='dashed', alpha=0.5)
    
    ax1.get_shared_x_axes().join(ax1, ax2)
    plots = p1 + p2
    labels = [l.get_label() for l in plots]
    ax2.legend(plots, labels, loc=1, fontsize=10)

    ax1.set_xlabel('Final SNR after a step increase from initial 1 dB', fontsize=12)
    ax1.set_title(title, fontsize=14)
    ax1.spines['right'].set_visible(False)
    ax1.spines['top'].set_visible(False)
    ax1.spines['left'].set_linewidth(0.5)
    ax1.spines['bottom'].set_linewidth(0.5)
    ax2.grid(axis='both', which='both', alpha=0.4)
    ax1.grid(axis='x', which='both', alpha=0.4)
    plt.savefig(out,dpi=300,bbox_inches='tight')


# Read in data for error rate model curves
snr = []
mcs0, mcs1, mcs2, mcs3, mcs4, mcs5, mcs6, mcs7, mcs8 = [], [], [], [], [], [], [], [], []
for line in open('scripts/error-model-reference-data.txt', 'r'):
    if line.startswith('#'):
        continue
    values = [float(s) for s in line.split()]
    snr.append(values[0])
    mcs0.append(values[1])
    mcs1.append(values[2])
    mcs2.append(values[3])
    mcs3.append(values[4])
    mcs4.append(values[5])
    mcs5.append(values[6])
    mcs6.append(values[7])
    mcs7.append(values[8])
    mcs8.append(values[9])

if (len(sys.argv) != 2):
    print("Missing figure specifier argument (figure-7a, figure-7b, etc.)")
    sys.exit(1)

if (sys.argv[1] == "figure-7a"):
    df = pd.read_csv("figure-7a.csv")
    df.columns = ["test", "manager", "timetoconverge"]
    df2 = pd.read_csv("figure-7a-noconvergence.csv")
    df2.columns = ["test", "manager", "timetoconverge"]
    df3 = pd.concat([df, df2], axis=0)
    plot_convergenceD(df3,"figure-7a.png", snr, mcs0, mcs1, mcs2, mcs3, mcs4, mcs5, mcs6, mcs7, mcs8,"MinstrelHt")
elif (sys.argv[1] == "figure-7b"):
    df = pd.read_csv("figure-7b.csv")
    df.columns = ["test", "manager", "timetoconverge"]
    df2 = pd.read_csv("figure-7b-noconvergence.csv")
    df2.columns = ["test", "manager", "timetoconverge"]
    df3 = pd.concat([df, df2], axis=0)
    plot_convergenceU(df3,"figure-7b.png", snr, mcs0, mcs1, mcs2, mcs3, mcs4, mcs5, mcs6, mcs7, mcs8,"MinstrelHt")
elif (sys.argv[1] == "figure-7c"):
    df = pd.read_csv("figure-7c.csv")
    df.columns = ["test", "manager", "timetoconverge"]
    df2 = pd.read_csv("figure-7c-noconvergence.csv")
    df2.columns = ["test", "manager", "timetoconverge"]
    df3 = pd.concat([df, df2], axis=0)
    plot_convergenceD(df3,"figure-7c.png", snr, mcs0, mcs1, mcs2, mcs3, mcs4, mcs5, mcs6, mcs7, mcs8,"ThompsonSampling")
elif (sys.argv[1] == "figure-7d"):
    df = pd.read_csv("figure-7d.csv")
    df.columns = ["test", "manager", "timetoconverge"]
    df2 = pd.read_csv("figure-7d-noconvergence.csv")
    df2.columns = ["test", "manager", "timetoconverge"]
    df3 = pd.concat([df, df2], axis=0)
    plot_convergenceU(df3,"figure-7d.png", snr, mcs0, mcs1, mcs2, mcs3, mcs4, mcs5, mcs6, mcs7, mcs8,"ThompsonSampling")
