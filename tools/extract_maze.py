#!/usr/bin/env python3
"""
Maze Node Extractor v2
Improved version with better path detection for Pac-Man mazes.

Usage:
    python extract_maze.py maze_input.png
"""

import cv2
import numpy as np
import sys
from pathlib import Path

def load_and_preprocess(image_path):
    """Load image and extract paths (black corridors)"""
    img = cv2.imread(str(image_path))
    if img is None:
        raise ValueError(f"Could not load image: {image_path}")
    
    # Convert to HSV to better isolate colors
    hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    
    # The maze has:
    # - Blue walls (high saturation, specific hue)
    # - Black paths (low value/brightness)
    # - Pink ghost house door
    
    # Method 1: Get dark areas (paths are black)
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    
    # Paths are dark (black), walls are bright (blue)
    _, paths = cv2.threshold(gray, 25, 255, cv2.THRESH_BINARY_INV)
    
    # Method 2: Also exclude the blue walls explicitly
    # Blue walls have high saturation
    lower_blue = np.array([100, 50, 50])
    upper_blue = np.array([140, 255, 255])
    blue_mask = cv2.inRange(hsv, lower_blue, upper_blue)
    
    # Combine: paths = dark areas AND NOT blue walls
    paths = cv2.bitwise_and(paths, cv2.bitwise_not(blue_mask))
    
    # Clean up with morphological operations
    kernel = np.ones((3, 3), np.uint8)
    paths = cv2.morphologyEx(paths, cv2.MORPH_CLOSE, kernel, iterations=2)
    paths = cv2.morphologyEx(paths, cv2.MORPH_OPEN, kernel, iterations=1)
    
    return img, paths

def skeletonize(binary):
    """Create skeleton using OpenCV's thinning"""
    return cv2.ximgproc.thinning(binary)

def find_decision_points(skeleton):
    """
    Find decision points by analyzing local neighborhood.
    A decision point has != 2 connected neighbors (intersections, corners, dead ends).
    """
    h, w = skeleton.shape
    nodes = []
    
    # 8-connectivity kernel for counting neighbors
    for y in range(1, h-1):
        for x in range(1, w-1):
            if skeleton[y, x] == 0:
                continue
            
            # Count 8-connected neighbors
            neighborhood = skeleton[y-1:y+2, x-1:x+2].copy()
            neighborhood[1, 1] = 0  # Exclude center
            neighbor_count = np.sum(neighborhood > 0)
            
            # 4-connected neighbors only (cardinal directions)
            cardinal_neighbors = [
                skeleton[y-1, x],  # Up
                skeleton[y+1, x],  # Down
                skeleton[y, x-1],  # Left
                skeleton[y, x+1]   # Right
            ]
            cardinal_count = sum(1 for n in cardinal_neighbors if n > 0)
            
            # Node if:
            # - Dead end (1 connection)
            # - Intersection (3+ connections)
            # - Also consider corners based on 8-connectivity
            if neighbor_count >= 3 or neighbor_count == 1:
                nodes.append((x, y, neighbor_count))
            elif neighbor_count == 2:
                # Check if it's a corner (not a straight line)
                # Straight line: neighbors are opposite each other
                up = skeleton[y-1, x] > 0
                down = skeleton[y+1, x] > 0
                left = skeleton[y, x-1] > 0
                right = skeleton[y, x+1] > 0
                
                is_horizontal_straight = left and right and not up and not down
                is_vertical_straight = up and down and not left and not right
                
                if not (is_horizontal_straight or is_vertical_straight):
                    # It's a corner or diagonal
                    # Only add as node if it's a clear corner
                    if cardinal_count == 2:
                        nodes.append((x, y, neighbor_count))
    
    return nodes

def cluster_nodes(nodes, min_distance=20):
    """Merge nodes that are too close together"""
    if not nodes:
        return []
    
    nodes = sorted(nodes, key=lambda n: (n[1], n[0]))
    clustered = []
    used = set()
    
    for i, (x1, y1, c1) in enumerate(nodes):
        if i in used:
            continue
        
        cluster = [(x1, y1, c1)]
        used.add(i)
        
        for j, (x2, y2, c2) in enumerate(nodes):
            if j in used:
                continue
            
            dist = np.sqrt((x2-x1)**2 + (y2-y1)**2)
            if dist < min_distance:
                cluster.append((x2, y2, c2))
                used.add(j)
        
        avg_x = int(np.mean([n[0] for n in cluster]))
        avg_y = int(np.mean([n[1] for n in cluster]))
        max_neighbors = max(n[2] for n in cluster)
        clustered.append((avg_x, avg_y, max_neighbors))
    
    return clustered

def trace_path(skeleton, start_x, start_y, direction, visited_globally):
    """
    Trace a path from start point in given direction until hitting a node or dead end.
    Direction: (dx, dy)
    Returns: end position or None if no path
    """
    dx, dy = direction
    x, y = start_x + dx, start_y + dy
    h, w = skeleton.shape
    
    steps = 0
    max_steps = max(h, w)
    
    while 0 <= x < w and 0 <= y < h and steps < max_steps:
        if skeleton[y, x] == 0:
            return None  # Hit wall
        
        steps += 1
        
        # Check if we've reached another decision point
        neighborhood = skeleton[max(0,y-1):min(h,y+2), max(0,x-1):min(w,x+2)].copy()
        if neighborhood.shape == (3, 3):
            neighborhood[1, 1] = 0
            neighbor_count = np.sum(neighborhood > 0)
            if neighbor_count != 2:  # Not a corridor = decision point
                return (x, y)
        
        # Continue along the path - prefer the same direction
        next_found = False
        
        # First try continuing straight
        nx, ny = x + dx, y + dy
        if 0 <= nx < w and 0 <= ny < h and skeleton[ny, nx] > 0:
            x, y = nx, ny
            next_found = True
        else:
            # Try perpendicular directions
            for new_dx, new_dy in [(dy, dx), (-dy, -dx), (-dx, -dy)]:
                nx, ny = x + new_dx, y + new_dy
                if 0 <= nx < w and 0 <= ny < h and skeleton[ny, nx] > 0:
                    prev_x, prev_y = x - dx, y - dy
                    if (nx, ny) != (prev_x, prev_y):  # Don't go back
                        x, y = nx, ny
                        dx, dy = new_dx, new_dy
                        next_found = True
                        break
        
        if not next_found:
            return (x, y)  # End of path
    
    return None

def build_connections(nodes, skeleton):
    """Build connection matrix between nodes"""
    h, w = skeleton.shape
    
    # Create lookup for finding node by position
    node_positions = {}
    for i, (x, y, _) in enumerate(nodes):
        # Store in a small radius
        for dy in range(-10, 11):
            for dx in range(-10, 11):
                pos = (x + dx, y + dy)
                if pos not in node_positions:
                    node_positions[pos] = i
    
    connections = [[-1, -1, -1, -1] for _ in nodes]  # RIGHT, DOWN, LEFT, UP
    directions = [(1, 0), (0, 1), (-1, 0), (0, -1)]
    
    for node_id, (nx, ny, _) in enumerate(nodes):
        for dir_idx, (dx, dy) in enumerate(directions):
            # Check if there's a path in this direction
            if not (0 <= ny + dy < h and 0 <= nx + dx < w):
                continue
            if skeleton[ny + dy, nx + dx] == 0:
                continue
            
            # Trace path to find connecting node
            end = trace_path(skeleton, nx, ny, (dx, dy), set())
            if end:
                end_x, end_y = end
                # Find which node is at this position
                if (end_x, end_y) in node_positions:
                    target_id = node_positions[(end_x, end_y)]
                    if target_id != node_id:
                        connections[node_id][dir_idx] = target_id
    
    return connections

def convert_to_grid(nodes, img_width, img_height, grid_cols=28, grid_rows=31):
    """Convert pixel coordinates to grid coordinates"""
    cell_w = img_width / grid_cols
    cell_h = img_height / grid_rows
    
    grid_nodes = []
    for x, y, nc in nodes:
        gx = int(x / cell_w)
        gy = int(y / cell_h)
        grid_nodes.append((gx, gy, nc))
    
    return grid_nodes

def generate_cpp_header(nodes, pixel_nodes, connections, output_path, img_width, img_height):
    """Generate C++ header with both grid and pixel coordinates"""
    
    header = f'''// Auto-generated by extract_maze.py
// Image size: {img_width}x{img_height}
// Do not edit manually

#ifndef MAZE_DATA_H
#define MAZE_DATA_H

#include <array>
#include <vector>

struct MazeNodeData {{
    int gridX, gridY;           // Grid coordinates
    int pixelX, pixelY;         // Pixel coordinates
    std::array<int, 4> neighbors; // RIGHT, DOWN, LEFT, UP (-1 if none)
}};

inline std::vector<MazeNodeData> getMazeNodes() {{
    return {{
'''
    
    for i, ((gx, gy, _), (px, py, _), conn) in enumerate(zip(nodes, pixel_nodes, connections)):
        neighbors_str = "{" + ", ".join(str(c) for c in conn) + "}"
        header += f'        {{{gx}, {gy}, {px}, {py}, {neighbors_str}}},  // Node {i}\n'
    
    header += '''    };
}

#endif // MAZE_DATA_H
'''
    
    with open(output_path, 'w') as f:
        f.write(header)
    
    print(f"Generated: {output_path}")

def visualize(img, skeleton, nodes, connections, output_path):
    """Create debug visualization with node numbers"""
    result = img.copy()
    
    # Overlay skeleton in green
    skeleton_color = cv2.cvtColor(skeleton, cv2.COLOR_GRAY2BGR)
    skeleton_color[skeleton > 0] = [0, 100, 0]
    result = cv2.addWeighted(result, 0.7, skeleton_color, 0.3, 0)
    
    # Draw connections
    for i, (x, y, _) in enumerate(nodes):
        for dir_idx, neighbor_id in enumerate(connections[i]):
            if neighbor_id >= 0:
                nx, ny, _ = nodes[neighbor_id]
                cv2.line(result, (x, y), (nx, ny), (0, 255, 0), 2)
    
    # Draw nodes with numbers
    for i, (x, y, nc) in enumerate(nodes):
        color = (0, 255, 255) if nc >= 3 else (255, 255, 0) if nc == 2 else (255, 0, 255)
        cv2.circle(result, (x, y), 8, color, -1)
        cv2.circle(result, (x, y), 8, (255, 255, 255), 1)
        # Draw node ID number
        cv2.putText(result, str(i), (x - 5, y + 4), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.35, (0, 0, 0), 2)
        cv2.putText(result, str(i), (x - 5, y + 4), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.35, (255, 255, 255), 1)
    
    cv2.imwrite(str(output_path), result)
    print(f"Saved: {output_path}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python extract_maze.py <maze_image.png>")
        sys.exit(1)
    
    image_path = Path(sys.argv[1])
    output_dir = image_path.parent
    
    print(f"Processing: {image_path}")
    
    img, paths = load_and_preprocess(image_path)
    h, w = img.shape[:2]
    print(f"Image: {w}x{h}")
    
    cv2.imwrite(str(output_dir / "maze_binary.png"), paths)
    
    skeleton = skeletonize(paths)
    cv2.imwrite(str(output_dir / "maze_skeleton.png"), skeleton)
    
    raw_nodes = find_decision_points(skeleton)
    print(f"Raw nodes: {len(raw_nodes)}")
    
    nodes = cluster_nodes(raw_nodes, min_distance=18)
    print(f"Clustered: {len(nodes)}")
    
    connections = build_connections(nodes, skeleton)
    
    grid_nodes = convert_to_grid(nodes, w, h)
    
    generate_cpp_header(grid_nodes, nodes, connections, output_dir / "maze_data.h", w, h)
    visualize(img, skeleton, nodes, connections, output_dir / "maze_debug.png")
    
    print(f"\nDone! Total nodes: {len(nodes)}")

if __name__ == "__main__":
    main()
