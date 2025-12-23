# Maze Graph Extraction Tools

Tools for extracting and processing node-based maze data from Pac-Man maze images using OpenCV.

## Overview

The classic Pac-Man maze is converted into a graph of 86 interconnected nodes for movement logic.

## Process

1. **Image Processing** (`extract_maze.py`)
   - Load maze image and convert to binary (paths vs walls)
   - Skeletonize paths to single-pixel width
   - Detect decision points (intersections, corners, dead ends)
   - Cluster nearby points to reduce noise

2. **Manual Refinement** 
   - `detect_red_nodes.py` - Add nodes marked with red dots
   - `remove_nodes.py` - Remove incorrect nodes
   - `cleanup_nodes.py` - Batch removal with renumbering

3. **Connection Building** (`connect_nodes.py`)
   - Auto-connect nodes on same row/column
   - Find nearest neighbor in each cardinal direction

4. **Validation**
   - `fix_bidirectional.py` - Ensure all connections are symmetric
   - `visualize_cleaned.py` - Generate debug visualization
   - `align_nodes.py` - Snap coordinates to grid

## Output

- `maze_data.h` - C++ header with 86 nodes, each containing:
  - Grid coordinates (28x31 grid)
  - Pixel coordinates (for rendering)
  - 4 neighbor IDs (RIGHT, DOWN, LEFT, UP)

## Tunnel Teleports

Nodes 84/85 are tunnel entries on the left edge that teleport to nodes 28/36 on the right edge.

## Setup

```bash
# Create conda environment (optional)
conda create -n maze-tools python=3.11 -y
conda activate maze-tools

# Install dependencies
pip install -r requirements.txt
```

## Usage

```bash
python extract_maze.py maze_input.png    # Initial extraction
python visualize_cleaned.py              # Generate debug image
```
