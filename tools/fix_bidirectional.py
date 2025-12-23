#!/usr/bin/env python3
"""Ensure all connections are bidirectional.
If A->B exists, then B->A should exist in opposite direction."""
import re

# Direction opposites: RIGHT(0)<->LEFT(2), DOWN(1)<->UP(3)
OPPOSITE = {0: 2, 1: 3, 2: 0, 3: 1}
DIR_NAMES = {0: 'RIGHT', 1: 'DOWN', 2: 'LEFT', 3: 'UP'}

# Load nodes
nodes = []
with open('maze_data.h', 'r') as f:
    for line in f:
        if line.strip().startswith('{') and '//' in line:
            data_part = line.split('//')[0].strip().rstrip(',')
            match = re.search(r'\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*\{(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\}\}', data_part)
            if match:
                gx, gy = int(match.group(1)), int(match.group(2))
                px, py = int(match.group(3)), int(match.group(4))
                neighbors = [int(match.group(i)) for i in range(5, 9)]
                nodes.append({'gx': gx, 'gy': gy, 'px': px, 'py': py, 'neighbors': neighbors})

print(f"Loaded {len(nodes)} nodes")

# Check and fix bidirectional connections
fixes = 0
for node_id, node in enumerate(nodes):
    for dir_idx, neighbor_id in enumerate(node['neighbors']):
        if neighbor_id >= 0 and neighbor_id < len(nodes):
            opposite_dir = OPPOSITE[dir_idx]
            neighbor = nodes[neighbor_id]
            
            # Check if neighbor points back to this node
            if neighbor['neighbors'][opposite_dir] != node_id:
                old_val = neighbor['neighbors'][opposite_dir]
                neighbor['neighbors'][opposite_dir] = node_id
                print(f"Fixed: Node {neighbor_id} {DIR_NAMES[opposite_dir]} was {old_val}, now points to {node_id}")
                fixes += 1

print(f"\nMade {fixes} fixes for bidirectional consistency")

# Generate header
header = '''// Maze node data (bidirectional)
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

print("Written bidirectional connections to maze_data.h")
