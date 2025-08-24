#!/usr/bin/env python3

import sys
import re
from typing import List, Tuple, Dict

class OutputVerifier:
    """Verifies test output based on custom rules."""
    
    def __init__(self):
        self.errors = []
    
    def verify_shell_exhaustion(self, actual_output: str) -> bool:
        """Verify shell exhaustion test case.
        
        Requirements:
        1. Must contain "Shoot" in the output (tanks must shoot)
        2. Must end with "Tie, both players have zero shells for 40 steps"
        """
        lines = actual_output.strip().split('\n')
        
        # Check if any line contains "Shoot"
        found_shoot = False
        for line in lines[:-1]:  # Check all but last line
            if "Shoot" in line:
                found_shoot = True
                break
        
        # Check if last line is the expected tie message
        last_line = lines[-1] if lines else ""
        correct_ending = last_line == "Tie, both players have zero shells for 40 steps"
        
        if not found_shoot:
            self.errors.append("No 'Shoot' action found in output")
        if not correct_ending:
            self.errors.append(f"Expected last line to be 'Tie, both players have zero shells for 40 steps', but got: {last_line}")
        
        return found_shoot and correct_ending
    
    def verify_chasing_wins(self, actual_output: str) -> bool:
        """Verify that the chasing algorithm (player 1) wins.
        
        Requirements:
        1. The game must end with player 1 winning
        2. At least one tank must have shot (to ensure it's not just movement)
        """
        lines = actual_output.strip().split('\n')
        if not lines:
            self.errors.append("Empty output")
            return False
            
        # Check if any tank shot
        found_shoot = False
        for line in lines[:-1]:  # Check all but last line
            if "Shoot" in line:
                found_shoot = True
                break
                
        if not found_shoot:
            self.errors.append("No 'Shoot' action found in output - tanks must engage in combat")
            return False
            
        # Check if player 1 won
        last_line = lines[-1]
        if not last_line.startswith("Player 1 won with"):
            self.errors.append(f"Expected player 1 (chasing) to win, but got: {last_line}")
            return False
            
        return True
    
    def get_errors(self) -> List[str]:
        """Get list of verification errors."""
        return self.errors

def main():
    if len(sys.argv) != 3:
        print("Usage: verify_output.py <actual_output> <expected_output>")
        sys.exit(1)
        
    verifier = OutputVerifier()
    if not verifier.verify_shell_exhaustion(sys.argv[1]):
        print("Verification failed!")
        for error in verifier.get_errors():
            print(f"Error: {error}")
        sys.exit(1)
    else:
        print("Verification passed!")
        sys.exit(0)

if __name__ == "__main__":
    main() 