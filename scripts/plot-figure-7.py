import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def read_txt(file_path):
    data = []
    with open(file_path, 'r') as f:
        for line in f:
            if line.strip():
                parts = line.split()
                if len(parts) == 3 and parts[2] != "N/A":
                    data.append((float(parts[0]), float(parts[2])))
    return zip(*data)  # Returns (x, y)

def plot_snr_vs_latency(managers, output_file):
    plt.figure(figsize=(10, 6))
    
    colors = {"MinstrelHt": "blue", "CARA": "green", "AARF": "orange"}
    
    for manager in managers:
        file_path = f"figure-7-{manager}.txt"
        try:
            x, y = read_txt(file_path)
            plt.plot(x, y, label=manager, marker='o', linestyle='-', color=colors[manager])
        except FileNotFoundError:
            print(f"File not found: {file_path}")

    plt.xlabel("SNR (dB)")
    plt.ylabel("Latency (seconds)")
    plt.title("SNR vs Latency for Minstrel, CARA, and AARF")
    plt.legend()
    plt.grid(True)
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Plot saved to {output_file}")

if __name__ == "__main__":
    output_file = "snr_vs_latency_plot.png"
    managers = ["MinstrelHt", "CARA", "AARF"]
    plot_snr_vs_latency(managers, output_file)
