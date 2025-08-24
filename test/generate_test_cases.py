#!/usr/bin/env python3

import os
from typing import Dict, List

class TestCaseGenerator:
    def __init__(self):
        self.base_dir = os.path.dirname(os.path.abspath(__file__))
        
    def generate_all_test_cases(self):
        """Generate all test cases for each category."""
        self.generate_file_system_tests()
        self.generate_header_error_tests()
        self.generate_dimension_mismatch_tests()
        self.generate_content_edge_case_tests()
        self.generate_game_logic_tests()
        
    def write_test_case(self, category: str, filename: str, content: str, expected_output: str):
        """Write a test case and its expected output."""
        # Create test map
        map_path = os.path.join(self.base_dir, 'maps', category, filename)
        with open(map_path, 'w') as f:
            f.write(content)
            
        # Create expected output
        output_path = os.path.join(self.base_dir, 'expected_outputs', 
                                 f"{os.path.splitext(filename)[0]}_expected.txt")
        with open(output_path, 'w') as f:
            f.write(expected_output)
            
    def generate_file_system_tests(self):
        """Generate file system related test cases."""
        # Empty file test
        self.write_test_case(
            'file_system',
            'empty_map.txt',
            '',
            'Error parsing input file: Empty map file\n'
        )
        
        # Binary file test will be created by the test runner
        
        # CRLF line endings test
        self.write_test_case(
            'file_system',
            'crlf_map.txt',
            'Test Map\r\nMaxSteps = 100\r\nNumShells = 5\r\nRows = 3\r\nCols = 3\r\n\r\n1 1\r\n   \r\n2 2\r\n',
            'Player 1 won with 2 tanks still alive\n'
        )
        
    def generate_header_error_tests(self):
        """Generate header error test cases."""
        # Missing MaxSteps
        self.write_test_case(
            'header_errors',
            'missing_maxsteps.txt',
            'Test Map\nNumShells = 5\nRows = 3\nCols = 3\n\n1 1\n   \n2 2\n',
            'Error parsing input file: Invalid parameter format: \n'
        )
        
        # Missing NumShells
        self.write_test_case(
            'header_errors',
            'missing_numshells.txt',
            'Test Map\nMaxSteps = 100\nRows = 3\nCols = 3\n\n1 1\n   \n2 2\n',
            'Error parsing input file: Invalid parameter format: \n'
        )
        
        # Invalid values
        self.write_test_case(
            'header_errors',
            'invalid_values.txt',
            'Test Map\nMaxSteps = abc\nNumShells = 5\nRows = -1\nCols = 3.14\n\n1 1\n   \n2 2\n',
            'Error parsing input file: Invalid value in parameter: MaxSteps = abc\n'
        )
        
    def generate_dimension_mismatch_tests(self):
        """Generate dimension mismatch test cases."""
        # Fewer rows than specified
        self.write_test_case(
            'dimension_mismatches',
            'fewer_rows.txt',
            'Test Map\nMaxSteps = 100\nNumShells = 5\nRows = 5\nCols = 3\n\n1 1\n   \n',
            'Player 1 won with 2 tanks still alive\n'
        )
        
        # More rows than specified
        self.write_test_case(
            'dimension_mismatches',
            'more_rows.txt',
            'Test Map\nMaxSteps = 100\nNumShells = 5\nRows = 2\nCols = 3\n\n1 1\n   \n2 2\nExtra\nExtra\n',
            'Player 1 won with 2 tanks still alive\n'
        )
        
    def generate_content_edge_case_tests(self):
        """Generate content edge case test cases."""
        # No tanks
        self.write_test_case(
            'content_edge_cases',
            'no_tanks.txt',
            'Test Map\nMaxSteps = 100\nNumShells = 5\nRows = 3\nCols = 3\n\n   \n   \n   \n',
            'Tie, both players have zero tanks\n'
        )
        
        # Only Player 1 tank
        self.write_test_case(
            'content_edge_cases',
            'only_player1.txt',
            'Test Map\nMaxSteps = 100\nNumShells = 5\nRows = 3\nCols = 3\n\n1  \n   \n   \n',
            'Player 1 won with 1 tanks still alive\n'
        )
        
    def generate_game_logic_tests(self):
        """Generate game logic test cases."""
        # Shell exhaustion test is now handled separately
        self.generate_shell_exhaustion_test()

    def generate_shell_exhaustion_test(self):
        """Generate shell exhaustion test case with custom verification logic."""
        # Set up algorithm configuration for this test
        config_path = os.path.join(self.base_dir, '..', 'build', 'algorithm_types.txt')
        with open(config_path, 'w') as f:
            f.write("chasing\nchasing\n")
        
        # Create test map
        map_path = os.path.join(self.base_dir, 'maps', 'game_logic', 'shell_exhaustion.txt')
        with open(map_path, 'w') as f:
            f.write('Test Map\nMaxSteps = 100\nNumShells = 1\nRows = 5\nCols = 5\n\n1   2\n     \n     \n     \n     \n')
        
        # Create a special verification file that indicates this needs custom verification
        output_path = os.path.join(self.base_dir, 'expected_outputs', 'shell_exhaustion_expected.txt')
        with open(output_path, 'w') as f:
            # First line must contain "Shoot" somewhere
            # Last line must be exactly this after 40 steps of no shells
            f.write('VERIFY_SHELL_EXHAUSTION\n')

def main():
    generator = TestCaseGenerator()
    generator.generate_all_test_cases()
    print("Test cases generated successfully!")

if __name__ == "__main__":
    main() 
