# Tank Battle Game

## Authors
- Amit Novick 314978222
- Tomer Yihya 203596192

This is a console-based tank battle game simulation where two or more AI-controlled tanks battle on a 2D board. The game includes walls, mines, and implements various game mechanics such as shooting, movement, and collision detection.

## Project Structure

The project includes the following files:

### Core Classes
- `Position.h` - Class representing a 2D position
- `Direction.h` - Enum and utilities for direction handling
- `GameObject.h` - Base class for all game objects
- `Tank.h` - Tank implementation with movement and shooting capabilities
- `Shell.h` - Artillery shell implementation
- `Wall.h` - Destructible wall implementation
- `Mine.h` - Mine implementation

### Game Logic
- `ActionRequest.h` - Enum for possible tank actions
- `GameState.h` - Manages the current state of the game board
- `TankAlgorithm.h` - Interface for tank AI algorithms
- `ChasingAlgorithm.h` - Advanced algorithm that uses pathfinding
- `GameManager.h` - Orchestrates the game flow

### Main and Build
- `main.cpp` - Entry point for the application
- `CMakeLists.txt` - CMake configuration file

### Documentation
- `bonus.txt` - Description of bonus features

## How to Build

### Using CMake
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Using GCC directly
```bash
g++ -std=c++20 src/*.h src/*.cpp src/common/*.h -o tanks_game
```


## How to Run

```bash
./tanks_game sample_board.txt
```

The program will read the input file, simulate the tank battle, and write the results to `output_<input_filename>.txt`. If there are any errors in the input file, they will be recorded in `input_errors.txt`.

## Input File Format

The input file should follow this format:
- The first non-comment line contains the board width and height
- The following symbols represent the game board with characters:
  - '1': Player 1's tank
  - '2': Player 2's tank
  - '#': Wall
  - '@': Mine
  - ' ' or any other character: Empty space

Lines starting with '//' are treated as comments and ignored.

## Output File Format

```
<ActionRequest>, <ActionRequest>
.
.
.
<end of game result>
```

where <ActionRequest> is value in:
- RotateRight45
- RotateRight90
- RotateLeft45
- RotateRight_90
- MoveForward
- MoveBackward
- Shoot
- GetBattleInfo


## Game Rules

- Tanks can move forward, backward, and rotate
- Tanks can shoot artillery shells
- Walls can be destroyed after two hits
- Mines explode when a tank steps on them
- If a tank is destroyed, the other player wins
- If both tanks run out of shells, the game ends after 40 additional steps

## Algorithms

The game includes two different tank control algorithms:

1. **ChasingAlgorithm**: An advanced algorithm that:
   - Uses BFS (Breadth-First Search) to find paths to the enemy tank
   - Actively pursues the enemy
   - Still implements the basic behaviors of shooting and evading

2. **RotatingAlgorithm**
3. **InteractiveAlgorithm**

both explained in the `bonus.txt` file


