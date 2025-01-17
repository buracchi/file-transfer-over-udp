import io
import os
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

WINDOW_SIZES = [1, 2, 4, 8, 16]
PLR_VALUES = [0.001, 0.01, 0.1]
TIMEOUT_SETTINGS = ["1s", "Adaptive"]
FILE_SIZES = ["1MB", "10MB", "100MB"]
CSV_FILE_PATH = "benchmark_results.csv"

# Initialize as empty - will be populated from file if not set
csv_data = ''''''


def is_raw_benchmark_data(df):
    """
    Detect if the dataframe contains raw benchmark data or already processed data
    
    Raw data has a single "Transfer Duration" column
    Processed data has "Transfer Duration Min", "Transfer Duration Avg", "Transfer Duration Max"
    """
    return ('Transfer Duration' in df.columns and 
            'Transfer Duration Min' not in df.columns and
            'Transfer Duration Avg' not in df.columns and
            'Transfer Duration Max' not in df.columns)


def process_raw_benchmark_data(df):
    """
    Process raw benchmark data to calculate min, avg, max transfer durations
    
    Args:
        df: DataFrame with raw benchmark data (has 'Transfer Duration' column)
        
    Returns:
        DataFrame with processed data (min, avg, max columns)
    """
    grouped = df.groupby(['File Size', 'Timeout', 'Window Size', 'Packet Loss Rate'])
    
    aggregated = grouped.agg({
        'Transfer Duration': ['min', 'mean', 'max']
    }).reset_index()
    
    aggregated.columns = [
        col[0] if col[1] == '' else f"{col[0]} {col[1]}"
        for col in aggregated.columns.values
    ]
    
    # Rename columns to match expected format
    aggregated = aggregated.rename(columns={
        'Transfer Duration min': 'Transfer Duration Min',
        'Transfer Duration mean': 'Transfer Duration Avg',
        'Transfer Duration max': 'Transfer Duration Max'
    })
    
    return aggregated


def load_csv_data():
    """Load CSV data from file or use the existing string if already populated"""
    global csv_data

    if csv_data:
        return csv_data

    with open(CSV_FILE_PATH, 'r') as file:
        csv_data = file.read()

    return csv_data


def load_data_from_csv_string():
    """Load data from CSV string into a pandas DataFrame"""
    csv_content = load_csv_data()

    lines = csv_content.split('\n')
    if lines[0].startswith('sep='):
        csv_content = '\n'.join(lines[1:])

    df = pd.read_csv(io.StringIO(csv_content))

    if is_raw_benchmark_data(df):
        df = process_raw_benchmark_data(df)

    numeric_cols = ('Window Size', 'Packet Loss Rate',
                    'Transfer Duration Min', 'Transfer Duration Avg', 'Transfer Duration Max')

    for col in numeric_cols:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')

    return df


def get_column_with_variants(df, base_name, variants=None):
    """
    Get the column that matches any of the possible naming variants
    
    Args:
        df: DataFrame to search in
        base_name: Base name of the column
        variants: List of possible variants for the column name
        
    Returns:
        The first matching column name or None if no match
    """
    if variants is None:
        # Default variants for Transfer Duration columns
        if base_name == 'Transfer Duration Avg':
            variants = ['Transfer Duration Avg', 'Transfer Duration mean', 'Transfer Duration Mean']
        elif base_name == 'Transfer Duration Min':
            variants = ['Transfer Duration Min', 'Transfer Duration min', 'Transfer Duration Min']
        elif base_name == 'Transfer Duration Max':
            variants = ['Transfer Duration Max', 'Transfer Duration max', 'Transfer Duration Max']
        else:
            variants = [base_name]
    
    for variant in variants:
        if variant in df.columns:
            return variant
    
    # If no match is found, print available columns to help debug
    print(f"Could not find column matching {base_name}. Available columns: {df.columns.tolist()}")
    return None


def create_dataframes_dict(df, file_size):
    """Convert the DataFrame into a nested dictionary by timeout and PLR"""
    dataframes = {}

    # Filter for the selected file size
    df_filtered = df[df['File Size'] == file_size]

    for timeout in TIMEOUT_SETTINGS:
        dataframes[timeout] = {}

        for plr in PLR_VALUES:
            # Filter data for this timeout and PLR
            df_subset = df_filtered[(df_filtered['Timeout'] == timeout) & (df_filtered['Packet Loss Rate'] == plr)]

            # Create a new DataFrame with the required structure
            if not df_subset.empty:
                # Get column names safely
                min_col = get_column_with_variants(df_subset, 'Transfer Duration Min')
                avg_col = get_column_with_variants(df_subset, 'Transfer Duration Avg')
                max_col = get_column_with_variants(df_subset, 'Transfer Duration Max')
                
                # Make sure all required columns are found
                if min_col and avg_col and max_col:
                    subset_data = {
                        'Window Size': df_subset['Window Size'].values,
                        'Min Transfer Duration': df_subset[min_col].values,
                        'Avg Transfer Duration': df_subset[avg_col].values,
                        'Max Transfer Duration': df_subset[max_col].values
                    }
                    dataframes[timeout][plr] = pd.DataFrame(subset_data).dropna()
                else:
                    print(f"Missing required columns for timeout={timeout}, plr={plr}")
                    dataframes[timeout][plr] = pd.DataFrame(columns=[
                        'Window Size', 'Min Transfer Duration', 'Avg Transfer Duration', 'Max Transfer Duration'
                    ])
            else:
                # Create empty DataFrame with the right columns if no data
                dataframes[timeout][plr] = pd.DataFrame(columns=[
                    'Window Size', 'Min Transfer Duration', 'Avg Transfer Duration', 'Max Transfer Duration'
                ])

    return dataframes


def create_plot(dataframes, file_size):
    """Create a figure and subplots for a specific file size"""
    fig, axes = plt.subplots(nrows=len(TIMEOUT_SETTINGS),
                             ncols=len(PLR_VALUES),
                             figsize=(15, 7),
                             sharex=True,
                             sharey=True)

    for row_idx, timeout in enumerate(TIMEOUT_SETTINGS):
        for col_idx, plr in enumerate(PLR_VALUES):
            ax = axes[row_idx, col_idx]
            df = dataframes[timeout][plr]

            if not df.empty:
                # Draw candlestick directly
                for i in range(len(df)):
                    x = df["Window Size"].iloc[i]
                    min_dur = df["Min Transfer Duration"].iloc[i]
                    avg_dur = df["Avg Transfer Duration"].iloc[i]
                    max_dur = df["Max Transfer Duration"].iloc[i]
                    # Draw vertical line for min to max
                    ax.plot([x, x], [min_dur, max_dur], color='grey', linewidth=4)
                    # Draw horizontal line for the average (candle body)
                    ax.plot([x - 0.2, x + 0.2], [avg_dur, avg_dur], color='black', linewidth=4)

                ax.set_xticks(WINDOW_SIZES)

            ax.set_yscale('log')
            ax.grid(True)
            ax.set_title(f"File Size: {file_size}, PLR: {plr}, Timeout: {timeout}")

    fig.supxlabel('Window Size (in Blocks of 1450 Bytes)')
    fig.supylabel('Transfer Duration (s)')

    return fig


def generate_latex_table(df, file_size):
    """Generate LaTeX table code for the given dataframe and file size"""
    latex_code = []
    
    latex_code.append("\\begin{figure}[H]")
    latex_code.append("    \\centering")
    latex_code.append("    \\begin{tabular}{|c|c|c|c|c|c|c|c|c|}")
    latex_code.append("        \\hline")
    latex_code.append("        \\rowcolor{tblhdrcolor}")
    latex_code.append("        \\multicolumn{1}{|c|}{\\textbf{Dimensione File}}")
    latex_code.append("                       & \\multicolumn{1}{|c|}{\\textbf{$T$}}")
    latex_code.append("                       & \\multicolumn{1}{|c|}{\\textbf{$N$}}")
    latex_code.append("                       & \\multicolumn{1}{|c|}{\\textbf{$p$}}")
    latex_code.append("                       & \\multicolumn{1}{|c|}{\\textbf{Durata (min)}}")
    latex_code.append("                       & \\multicolumn{1}{|c|}{\\textbf{Durata (avg)}}")
    latex_code.append("                       & \\multicolumn{1}{|c|}{\\textbf{Durata (max)}}")
    latex_code.append("        \\\\\\hline")
    
    # Filter for the current file size
    df_filtered = df[df['File Size'] == file_size]
    
    # Get column names
    min_col = get_column_with_variants(df_filtered, 'Transfer Duration Min')
    avg_col = get_column_with_variants(df_filtered, 'Transfer Duration Avg')
    max_col = get_column_with_variants(df_filtered, 'Transfer Duration Max')
    
    # Sort by timeout, packet loss rate, and window size for consistent ordering
    df_sorted = df_filtered.sort_values(by=['Timeout', 'Packet Loss Rate', 'Window Size'])
    
    # Add rows for each data point
    for _, row in df_sorted.iterrows():
        timeout = row['Timeout']
        window_size = int(row['Window Size'])
        plr = row['Packet Loss Rate']
        min_dur = row[min_col]
        avg_dur = row[avg_col]
        max_dur = row[max_col]
        
        # Format T column: 1s or A (for Adaptive)
        t_value = timeout if timeout == "1s" else "$A$"
        
        # Create the table row
        row_text = f"        {file_size} & {t_value} & {window_size} & {plr} & {min_dur:.3f} & {avg_dur:.3f} & {max_dur:.3f}"
        latex_code.append(f"{row_text}\\\\\\hline")
    
    # Figure closing with image reference
    latex_code.append("    \\end{tabular}")
    latex_code.append("    \\vspace{1cm}")
    latex_code.append(f"    \\includegraphics[width=1\\textwidth]{{benchmark{file_size}.png}}")
    latex_code.append("\\end{figure}")
    
    return "\n".join(latex_code)


def main():
    df = load_data_from_csv_string()
    for file_size in FILE_SIZES:
        dataframes = create_dataframes_dict(df, file_size)
        fig = create_plot(dataframes, file_size)
        fig.tight_layout()
        fig.savefig("benchmark" + file_size, bbox_inches='tight')
        plt.show()
    plt.close()
    with open("benchmark.tex", 'w') as f:
        f.write("\n".join(map(lambda size: generate_latex_table(df, size) + "\n", FILE_SIZES)))


if __name__ == "__main__":
    main()
