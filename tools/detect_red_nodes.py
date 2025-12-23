#!/usr/bin/env python3
"""Detect red dots in image and add them as new nodes to maze_data.h"""

import cv2
import numpy as np
import re

# Load image with red dots
img = cv2.imread('maze_cleaned_debug_with_missing_nodes.png')
if img is None:
    print("ERROR: Could not load image")
    exit(1)

# Convert to HSV for better color detection
hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

# Red color in HSV (red wraps around 0, so two ranges)
lower_red1 = np.array([0, 100, 100])
upper_red1 = np.array([10, 255, 255])
lower_red2 = np.array([160, 100, 100])
upper_red2 = np.array([180, 255, 255])

mask1 = cv2.inRange(hsv, lower_red1, upper_red1)
mask2 = cv2.inRange(hsv, lower_red2, upper_red2)
red_mask = cv2.bitwise_or(mask1, mask2)

# Find contours of red areas
contours, _ = cv2.findContours(red_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

# Get center of each red dot
new_nodes = []
for contour in contours:
    M = cv2.moments(contour)
    if M["m00"] > 0:
        cx = int(M["m10"] / M["m00"])
        cy = int(M["m01"] / M["m00"])
        new_nodes.append((cx, cy))

print(f"Found {len(new_nodes)} red dots at positions:")
for i, (x, y) in enumerate(new_nodes):
    # Convert to grid coordinates (image is 926x1024, grid is ~28x31)
    gx = int(x / (926 / 28))
    gy = int(y / (1024 / 31))
    print(f"  Red dot {i}: pixel ({x}, {y}) -> grid ({gx}, {gy})")

# Load existing nodes from maze_data.h
existing_nodes = []
with open('maze_data.h', 'r') as f:
    for line in f:
        if line.strip().startswith('{') and 'Node' in line:
            match = re.search(r'\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*\{(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\}\}', line)
            if match:
                gx, gy, px, py = int(match.group(1)), int(match.group(2)), int(match.group(3)), int(match.group(4))
                neighbors = [int(match.group(i)) for i in range(5, 9)]
                existing_nodes.append({'gx': gx, 'gy': gy, 'px': px, 'py': py, 'neighbors': neighbors})

print(f"\nExisting nodes: {len(existing_nodes)}")

# Add new nodes (with no connections yet - you'll need to set these manually)
next_id = len(existing_nodes)
for px, py in new_nodes:
    gx = int(px / (926 / 28))
    gy = int(py / (1024 / 31))
    existing_nodes.append({'gx': gx, 'gy': gy, 'px': px, 'py': py, 'neighbors': [-1, -1, -1, -1], 'is_new': True})

# Generate new header
header = '''// Auto-generated with added red-marked nodes
// Do not edit manually

#ifndef MAZE_DATA_H
#define MAZE_DATA_H

#include <array>
#include <vector>

struct MazeNodeData {
    int gridX, gridY;           // Grid coordinates
    int pixelX, pixelY;         // Pixel coordinates
    std::array<int, 4> neighbors; // RIGHT, DOWN, LEFT, UP (-1 if none)
};

inline std::vector<MazeNodeData> getMazeNodes() {
    return {
'''

for i, node in enumerate(existing_nodes):
    neighbors_str = '{' + ', '.join(str(n) for n in node['neighbors']) + '}'
    marker = " <- NEW" if node.get('is_new') else ""
    header += f'        {{{node["gx"]}, {node["gy"]}, {node["px"]}, {node["py"]}, {neighbors_str}}},  // Node {i}{marker}\n'

header += '''    };
}

#endif // MAZE_DATA_H
'''

with open('maze_data_with_new.h', 'w') as f:
    f.write(header)

print(f"\nGenerated: maze_data_with_new.h with {len(existing_nodes)} total nodes ({len(new_nodes)} new)")
print("\nNOTE: New nodes have no connections (-1). You need to manually set their neighbors!")

# Also save debug image showing detected red dots
debug = img.copy()
for px, py in new_nodes:
    cv2.circle(debug, (px, py), 15, (0, 0, 255), 3)
    cv2.circle(debug, (px, py), 5, (255, 255, 255), -1)
cv2.imwrite('detected_red_dots.png', debug)
print("Saved: detected_red_dots.png")
