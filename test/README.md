# Tank Battle Testing Framework

This directory contains a comprehensive testing framework for the Tank Battle game, implementing the testing methodology outlined in the assignment.

## Directory Structure

```
test/
├── maps/                      # Test map files
│   ├── file_system/          # File system related tests
│   ├── header_errors/        # Header format and validation tests
│   ├── dimension_mismatches/ # Board dimension mismatch tests
│   ├── content_edge_cases/   # Game content edge cases
│   └── game_logic/          # Game mechanics tests
├── expected_outputs/         # Expected output files for each test
├── test_runner.sh           # Main test execution script
├── verify_output.py         # Output verification tool
└── generate_test_cases.py   # Test case generator
```

## Test Categories

1. **File System Tests**
   - Empty files
   - Binary files
   - Different line endings (CRLF/LF)
   - Permission issues

2. **Header Error Tests**
   - Missing required fields
   - Invalid values
   - Incorrect order
   - Malformed lines

3. **Dimension Mismatch Tests**
   - Fewer/more rows than specified
   - Shorter/longer lines
   - Zero dimensions

4. **Content Edge Cases**
   - No tanks
   - Single player tanks
   - Invalid tank positions
   - Tank-mine collisions

5. **Game Logic Tests**
   - Tank collisions
   - Shell exhaustion
   - Movement validation
   - Win/tie conditions

## Running Tests

1. Generate test cases:
   ```bash
   ./generate_test_cases.py
   ```

2. Run all tests:
   ```bash
   ./test_runner.sh
   ```

3. Run specific test category:
   ```bash
   ./test_runner.sh maps/file_system/
   ```

4. Verify single output:
   ```bash
   ./verify_output.py test_output.txt expected_outputs/test_name_expected.txt
   ```

## Adding New Tests

1. Create a new test map file in the appropriate category directory under `maps/`
2. Create the corresponding expected output file in `expected_outputs/`
3. Or modify `generate_test_cases.py` to generate the new test case

## Test Output Format

Each test produces:
1. Success/failure status
2. Detailed error messages if failed
3. Diff between expected and actual output

## Error Handling

The framework verifies:
1. Input file format
2. Game mechanics
3. Output format
4. Win/tie conditions
5. Error messages

## Verification Rules

1. Action format: `Action[, Action]*`
2. Valid actions: MoveForward, MoveBackward, RotateLeft90, RotateRight90, RotateLeft45, RotateRight45, Shoot, GetBattleInfo, DoNothing
3. Status modifiers: (ignored), (killed)
4. Final line format: 
   - `Player <X> won with <Y> tanks still alive`
   - `Tie, both players have zero tanks`
   - `Tie, reached max steps = <N>, player 1 has <X> tanks, player 2 has <Y> tanks`
   - `Tie, both players have zero shells for 40 steps` 