#!/usr/bin/env python3
"""Auto-connect nodes based on grid positions.
Nodes on same row/column that are adjacent (no node between) get connected."""

import re

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
                nodes.append({'id': node_id, 'gx': gx, 'gy': gy, 'px': px, 'py': py, 
                              'neighbors': [-1, -1, -1, -1]})  # Reset connections

print(f"Loaded {len(nodes)} nodes")

# Build grid lookup: (gx, gy) -> node_id
grid_lookup = {}
for node in nodes:
    grid_lookup[(node['gx'], node['gy'])] = node['id']

# For each node, find nearest neighbor in each direction
# Directions: RIGHT(0), DOWN(1), LEFT(2), UP(3)
for node in nodes:
    gx, gy = node['gx'], node['gy']
    
    # RIGHT: find closest node with same gy, larger gx
    right_candidates = [(n['gx'], n['id']) for n in nodes 
                        if n['gy'] == gy and n['gx'] > gx]
    if right_candidates:
        closest = min(right_candidates, key=lambda x: x[0])
        node['neighbors'][0] = closest[1]
    
    # DOWN: find closest node with same gx, larger gy
    down_candidates = [(n['gy'], n['id']) for n in nodes 
                       if n['gx'] == gx and n['gy'] > gy]
    if down_candidates:
        closest = min(down_candidates, key=lambda x: x[0])
        node['neighbors'][1] = closest[1]
    
    # LEFT: find closest node with same gy, smaller gx
    left_candidates = [(n['gx'], n['id']) for n in nodes 
                       if n['gy'] == gy and n['gx'] < gx]
    if left_candidates:
        closest = max(left_candidates, key=lambda x: x[0])
        node['neighbors'][2] = closest[1]
    
    # UP: find closest node with same gx, smaller gy
    up_candidates = [(n['gy'], n['id']) for n in nodes 
                     if n['gx'] == gx and n['gy'] < gy]
    if up_candidates:
        closest = max(up_candidates, key=lambda x: x[0])
        node['neighbors'][3] = closest[1]

# Count connections
total_connections = sum(1 for n in nodes for x in n['neighbors'] if x >= 0)
print(f"Created {total_connections} connections")

# Generate header
header = '''// Maze node data (auto-connected)
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

print("Written connected nodes to maze_data.h")
