"""Vector Analysis Tool"""

import matplotlib.pyplot as plt
import re
import time
from collections import defaultdict
from tkinter import filedialog

detection_area = {
"D0": {
    "x": -1.5,
    "y": 2.7999999523162842
  },
  "D1": {
    "x": -1.5,
    "y": 1.6000000238418579
  },
  "D2": {
    "x": 0.5,
    "y": 1.6000000238418579
  },
  "D3": {
    "x": 0.5,
    "y": 2.7999999523162842
  }
}

def parse_log_entries(lines):
    entry_pattern = re.compile(r"Target (\d+) previous point: \(([-\d.]+), ([-\d.]+)\) \| entry point: \(([-\d.]+), ([-\d.]+)\) \| exit point: \(([-\d.]+), ([-\d.]+)\) .* \| \[ *([A-Z]+) *\]")
    decision_pattern = re.compile(r"Target (\d+) (entered|exited|gave up (entering|exiting)) the room")
    
    detections = []
    
    for i in range(len(lines) - 1):
        entry_match = entry_pattern.search(lines[i])
        decision_match = decision_pattern.search(lines[i + 1])
        
        if entry_match and decision_match:
            target = int(entry_match.group(1))
            entry_x, entry_y = float(entry_match.group(4)), float(entry_match.group(5))
            exit_x, exit_y = float(entry_match.group(6)), float(entry_match.group(7))
            status = entry_match.group(8).strip()
            action = decision_match.group(2)  # 'entered' or 'exited'
            
            detections.append((target, entry_x, entry_y, exit_x, exit_y, status, action))
    
    return detections

def update_plot(fig, ax, data):
    ax.clear()
    ax.set_xlim(-5, 5)
    ax.set_ylim(0, 8)
    ax.set_title(f"Target {data['target']}")
    
    # Add detection area as a semi-transparent red rectangle
    x_vals = [detection_area["D0"]["x"], detection_area["D1"]["x"], detection_area["D2"]["x"], detection_area["D3"]["x"]]
    y_vals = [detection_area["D0"]["y"], detection_area["D1"]["y"], detection_area["D2"]["y"], detection_area["D3"]["y"]]
    ax.fill(x_vals + [x_vals[0]], y_vals + [y_vals[0]], 'red', alpha=0.3)
    
    for (ex, ey, en_x, en_y, status, action) in data['points']:
        color = 'green' if status == 'TRUSTED' else 'red'
        color = 'black' if action == 'gave up entering' or action == 'gave up exiting' else color
        if action == 'entered':
            ax.arrow(en_x, en_y, ex - en_x, ey - en_y, head_width=0.2, head_length=0.3, fc=color, ec=color)
        else:
            ax.arrow(ex, ey, (ex - en_x) * 0.5, (ey - en_y) * 0.5, head_width=0.2, head_length=0.3, fc=color, ec=color, linestyle='dashed')
    
    fig.canvas.draw()

def visualize_log(log_file):
    plt.ion()
    figures = {i: plt.figure(figsize=(5, 5)) for i in range(5)}
    axes = {i: figures[i].add_subplot(1, 1, 1) for i in range(5)}
    target_data = {i: {'target': i, 'points': []} for i in range(5)}
    
    with open(log_file, 'r') as f:
        lines = f.readlines()
    
    detections = parse_log_entries(lines)
    
    for target, entry_x, entry_y, exit_x, exit_y, status, action in detections:
        target_data[target]['points'].append((exit_x, exit_y, entry_x, entry_y, status, action))
    
    while True:
        for i in range(5):
            update_plot(figures[i], axes[i], target_data[i])
        
        plt.pause(30)
        
        

def main():
    log_file = filedialog.askopenfilename(title="Selecione o arquivo LOG")  # Nome do arquivo de log
    visualize_log(log_file)

if __name__ == "__main__":
    main()
