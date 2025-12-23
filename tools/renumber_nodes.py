#!/usr/bin/env python3
"""Fix node numbering to be sequential"""
import re

nodes = []
with open('maze_data.h', 'r') as f:
    for line in f:
        if line.strip().startswith('{') and '//' in line:
            # Extract the data part (before //)
            data_part = line.split('//')[0].strip().rstrip(',')
            match = re.search(r'\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*\{(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\}\}', data_part)
            if match:
                gx, gy = int(match.group(1)), int(match.group(2))
                px, py = int(match.group(3)), int(match.group(4))
                neighbors = [int(match.group(i)) for i in range(5, 9)]
                nodes.append({'gx': gx, 'gy': gy, 'px': px, 'py': py, 'neighbors': neighbors})

print(f"Loaded {len(nodes)} nodes")

# Generate header with correct sequential numbering
header = '''// Maze node data (renumbered)
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
for i, node in enumerate(nodes):
    n = '{' + ', '.join(str(x) for x in node['neighbors']) + '}'
    header += f'        {{{node["gx"]}, {node["gy"]}, {node["px"]}, {node["py"]}, {n}}},  // Node {i}\n'

header += '''    };
}

#endif
'''

with open('maze_data.h', 'w') as f:
    f.write(header)

print(f"Written {len(nodes)} nodes with sequential numbering (0-{len(nodes)-1})")
