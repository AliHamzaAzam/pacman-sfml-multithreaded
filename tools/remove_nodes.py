#!/usr/bin/env python3
"""Remove specific nodes and renumber"""
import re

nodes_to_remove = {35, 36, 52}

nodes = []
with open('maze_data.h', 'r') as f:
    for line in f:
        if line.strip().startswith('{') and 'Node' in line:
            match = re.search(r'\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*\{(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\}\}', line)
            node_id_match = re.search(r'Node (\d+)', line)
            if match and node_id_match:
                node_id = int(node_id_match.group(1))
                gx, gy, px, py = int(match.group(1)), int(match.group(2)), int(match.group(3)), int(match.group(4))
                neighbors = [int(match.group(i)) for i in range(5, 9)]
                nodes.append({'id': node_id, 'gx': gx, 'gy': gy, 'px': px, 'py': py, 'neighbors': neighbors})

print(f"Loaded {len(nodes)} nodes")
print(f"Removing nodes: {nodes_to_remove}")

# Build old->new mapping
old_to_new = {}
kept = []
new_id = 0
for node in nodes:
    if node['id'] not in nodes_to_remove:
        old_to_new[node['id']] = new_id
        node['new_id'] = new_id
        kept.append(node)
        new_id += 1

# Update neighbor references
for node in kept:
    node['neighbors'] = [old_to_new.get(n, -1) if n >= 0 else -1 for n in node['neighbors']]

# Generate header
header = '''// Maze node data (cleaned)
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
for node in kept:
    n = '{' + ', '.join(str(x) for x in node['neighbors']) + '}'
    header += f'        {{{node["gx"]}, {node["gy"]}, {node["px"]}, {node["py"]}, {n}}},  // Node {node["new_id"]}\n'

header += '''    };
}

#endif
'''

with open('maze_data.h', 'w') as f:
    f.write(header)

print(f"Written {len(kept)} nodes to maze_data.h")
