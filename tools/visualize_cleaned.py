#!/usr/bin/env python3
"""Visualize cleaned nodes on maze image"""

import cv2
import re

# Read node data from cleaned header
nodes = []
with open('maze_data.h', 'r') as f:
    for line in f:
        if line.strip().startswith('{') and 'Node' in line:
            # Parse: {gx, gy, px, py, {n1, n2, n3, n4}},  // Node N (was M)
            match = re.search(r'\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*\{(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\}\}', line)
            if match:
                gx, gy, px, py = int(match.group(1)), int(match.group(2)), int(match.group(3)), int(match.group(4))
                neighbors = [int(match.group(i)) for i in range(5, 9)]
                # Get new node ID
                node_id_match = re.search(r'Node (\d+)', line)
                node_id = int(node_id_match.group(1)) if node_id_match else len(nodes)
                nodes.append((node_id, px, py, neighbors))

print(f"Loaded {len(nodes)} nodes")

# Load maze image
img = cv2.imread('maze_input.png')

# Draw connections first
for node_id, px, py, neighbors in nodes:
    # RIGHT=0, DOWN=1, LEFT=2, UP=3
    for dir_idx, neighbor_id in enumerate(neighbors):
        if neighbor_id >= 0 and neighbor_id < len(nodes):
            nx, ny = nodes[neighbor_id][1], nodes[neighbor_id][2]
            cv2.line(img, (px, py), (nx, ny), (0, 255, 0), 2)

# Draw nodes with numbers
for node_id, px, py, neighbors in nodes:
    cv2.circle(img, (px, py), 10, (0, 255, 255), -1)
    cv2.circle(img, (px, py), 10, (255, 255, 255), 1)
    cv2.putText(img, str(node_id), (px - 7, py + 5), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 0, 0), 2)
    cv2.putText(img, str(node_id), (px - 7, py + 5), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 255), 1)

cv2.imwrite('maze_cleaned_debug.png', img)
print("Saved: maze_cleaned_debug.png")
