#!/usr/bin/env python3
"""Align nodes so lines are perfectly horizontal/vertical"""
import re

# L grid-based standard positions (based on grid size 28x31 in 926x1024 image)
# Cell size: 926/28 = 33.07, 1024/31 = 33.03
CELL_W = 926 / 28
CELL_H = 1024 / 31

def grid_to_pixel(gx, gy):
    """Convert grid coords to aligned pixel coords"""
    return int(gx * CELL_W + CELL_W/2), int(gy * CELL_H + CELL_H/2)

# Load nodes
nodes = []
with open('maze_data.h', 'r') as f:
    for line in f:
        if line.strip().startswith('{') and 'Node' in line:
            match = re.search(r'\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*\{(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\}\}', line)
            node_id_match = re.search(r'Node (\d+)', line)
            if match and node_id_match:
                node_id = int(node_id_match.group(1))
                gx, gy = int(match.group(1)), int(match.group(2))
                px, py = int(match.group(3)), int(match.group(4))
                neighbors = [int(match.group(i)) for i in range(5, 9)]
                # Recalculate pixel coords based on grid
                new_px, new_py = grid_to_pixel(gx, gy)
                nodes.append({'id': node_id, 'gx': gx, 'gy': gy, 'px': new_px, 'py': new_py, 'neighbors': neighbors})

print(f"Aligned {len(nodes)} nodes to grid")

# Generate header
header = '''// Maze node data (grid-aligned)
#ifndef MAZE_DATA_H
#define MAZE_DATA_H

#include <array>
#include <vector>

struct MazeNodeData {
    int gridX, gridY;
    int pixelX, pixelY;
    std::array<int, 4> neighbors; // RIGHT, DOWN, LEFT, UP
};

inline std::vector<MazeNodeData> getMazeNodes() {
    return {
'''
for node in nodes:
    n = '{' + ', '.join(str(x) for x in node['neighbors']) + '}'
    header += f'        {{{node["gx"]}, {node["gy"]}, {node["px"]}, {node["py"]}, {n}}},  // Node {node["id"]}\n'

header += '''    };
}

#endif
'''

with open('maze_data.h', 'w') as f:
    f.write(header)

print("Written grid-aligned nodes to maze_data.h")
