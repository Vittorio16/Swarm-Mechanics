import pandas as pd
import matplotlib.pyplot as plt
import os

# ==========================================
# CONFIGURATION
# ==========================================

# 1. Choose the column you want to graph (e.g., "Total", "Observation", "Cleanup", "Move")
COLUMNS = {"Buckets","Observation","Think","Move","Cleanup","Total"}


# 2. Define the files you want to compare and the labels for the legend
# The format is: {"Label for Legend": "filename.csv"}
# This example compares all versions for the "Prey Only" scenario
FILES_TO_PLOT = {
    "Original CPU": "data/profiling_results_cpu_predatori.csv",
    "CPU (SoA)": "data/profiling_results_gpuready_cpu_predatori.csv",
    "CPU (SoA, no Vectors)": "data/profiling_results_cpu_soa_predatori.csv",
    "GPU (V1)": "data/profiling_results_gpuV1_predatori.csv",
    "GPU (V2)": "data/profiling_results_gpuV2_predatori.csv"
}

# ==========================================
# GRAPHING LOGIC
# ==========================================

def plot_column_across_files(files_dict, target_column, title, save_filename):
    plt.figure(figsize=(10, 6))
    
    # Markers to distinguish the lines easily
    markers = ['o', 's', '^', 'd', 'v', '<', '>']
    
    for i, (label, filename) in enumerate(files_dict.items()):
        if not os.path.exists(filename):
            print(f"Warning: File '{filename}' not found. Skipping...")
            continue
            
        # Load the CSV
        df = pd.read_csv(filename)
        
        # Ensure the requested column exists in the file
        if target_column not in df.columns:
            print(f"Warning: Column '{target_column}' not found in {filename}. Skipping...")
            continue
            
        # Calculate Total Agents for the X-axis
        df['TotalAgents'] = df['NumPrey'] + df['NumPredators']
        
        # Plot the line
        plt.plot(
            df['TotalAgents'], 
            df[target_column], 
            label=label, 
            marker=markers[i % len(markers)], 
            linewidth=2, 
            markersize=6
        )

    # Set logarithmic scales (Base 2 for agents, Base 10 for time)
    plt.xscale('log', base=2)
    plt.yscale('log', base=10)
    
    # Labels and formatting
    plt.title(title, fontsize=14, pad=15)
    plt.xlabel('Total Number of Agents (Log2 Scale)', fontsize=12)
    plt.ylabel(f'{target_column} Time (µs) (Log10 Scale)', fontsize=12)
    
    # Grid lines for readability
    plt.grid(True, which="major", ls="-", alpha=0.6)
    plt.grid(True, which="minor", ls="--", alpha=0.3)
    
    plt.legend(fontsize=10)
    plt.tight_layout()
    
    if save_filename:
        plt.savefig(save_filename, dpi=300, bbox_inches='tight')
        print(f"Graph saved successfully as: {save_filename}")
        
    # plt.show()

if __name__ == "__main__":
    for target_column in COLUMNS:
        # Set the title of the graph
        GRAPH_TITLE = f"{target_column} Time Comparison: Predator"
        SAVE_FILENAME = f"graphs/{target_column}/predator.png"
        
        plot_column_across_files(FILES_TO_PLOT, target_column, GRAPH_TITLE, SAVE_FILENAME)