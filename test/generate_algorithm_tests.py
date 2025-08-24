#!/usr/bin/env python3

import os
from typing import List, Tuple

class AlgorithmTestGenerator:
    """Generates test cases for comparing algorithms."""
    
    def __init__(self, base_dir: str = "."):
        """Initialize the test generator."""
        self.base_dir = base_dir
        self.maps_dir = os.path.join(base_dir, "maps", "algorithm_comparison")
        self.expected_dir = os.path.join(base_dir, "expected_outputs")
        
        # Create directories if they don't exist
        os.makedirs(self.maps_dir, exist_ok=True)
        os.makedirs(self.expected_dir, exist_ok=True)
        
        # Set algorithm configuration
        self.write_algorithm_config()
    
    def write_algorithm_config(self):
        """Write the algorithm configuration file."""
        config_path = os.path.join(self.base_dir, "..", "build", "algorithm_types.txt")
        with open(config_path, 'w') as f:
            f.write("chasing\nrotating")
    
    def write_test_case(self, name: str, map_content: str, expected_winner: int):
        """Write a test case and its expected output."""
        # Write map file
        map_path = os.path.join(self.maps_dir, f"{name}.txt")
        with open(map_path, 'w') as f:
            f.write(map_content)
        
        # Write expected output - we only care that player 1 (chasing) wins
        output_path = os.path.join(self.expected_dir, f"{name}_expected.txt")
        with open(output_path, 'w') as f:
            f.write("VERIFY_CHASING_WINS\n")
    
    def generate_all_tests(self):
        """Generate all algorithm comparison test cases."""
        # Test 1: Simple direct path
        self.write_test_case(
            "direct_path",
            "Direct Path Test\nMaxSteps = 1000\nNumShells = 10\nRows = 5\nCols = 10\n"
            "##########\n"
            "#1      2#\n"
            "#        #\n"
            "#        #\n"
            "##########\n",
            1  # Player 1 (chasing) should win
        )
        
        # Test 2: Maze with multiple paths
        self.write_test_case(
            "maze_paths",
            "Maze Test\nMaxSteps = 2000\nNumShells = 15\nRows = 7\nCols = 15\n"
            "###############\n"
            "#1  #   #    #\n"
            "# # # # # ## #\n"
            "#   #   #    #\n"
            "### ### #### #\n"
            "#          2 #\n"
            "###############\n",
            1
        )
        
        # Test 3: Open battlefield with mines
        self.write_test_case(
            "mine_field",
            "Mine Field Test\nMaxSteps = 1500\nNumShells = 20\nRows = 6\nCols = 12\n"
            "############\n"
            "#1   @    #\n"
            "#  @   @  #\n"
            "#   @ @   #\n"
            "#  @   2  #\n"
            "############\n",
            1
        )
        
        # Test 4: Limited shells test
        self.write_test_case(
            "limited_shells",
            "Limited Shells Test\nMaxSteps = 2000\nNumShells = 3\nRows = 5\nCols = 10\n"
            "##########\n"
            "#1      #\n"
            "#   @   #\n"
            "#      2#\n"
            "##########\n",
            1
        )

def main():
    generator = AlgorithmTestGenerator()
    generator.generate_all_tests()

if __name__ == "__main__":
    main() 
