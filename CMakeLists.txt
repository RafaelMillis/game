cmake_minimum_required(VERSION 3.10)

# --- Start of Compiler Specification (Less Portable) ---
# Attempt to force g++ as the C++ compiler.
# This should come BEFORE the project() command.
# WARNING: This reduces portability and may fail if g++ is not found
#          or has a different name (e.g., g++-11) on the system.
set(CMAKE_CXX_COMPILER "g++" CACHE STRING "C++ compiler" FORCE)
# --- End of Compiler Specification ---

# Declare the project and specify the language since compiler is set manually
project(TanksBattleGame LANGUAGES CXX)

# Specify the C++ standard (handles -std=c++20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the desired warning and error flags
add_compile_options(-Wall -Wextra -Werror -pedantic)

# Original output directory setting
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)

# Add executable target
add_executable(
        tanks_game
        src/main.cpp
        src/GameManager.cpp
        src/MyTankAlgorithmFactory.cpp
        src/ChasingAlgorithm.cpp
        src/Tank.cpp
)

# Include directories
target_include_directories(tanks_game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(tanks_game SYSTEM PRIVATE /usr/include/c++/11)
add_compile_options(-Wno-error=restrict -Wno-restrict)
