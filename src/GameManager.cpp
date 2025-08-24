#include <cstddef>         // For size_t
#include <memory>          // For std::unique_ptr, std::shared_ptr
#include <string>          // For std::string
#include <vector>          // For std::vector
#include <unordered_map>   // For std::unordered_map
#include <fstream>         // For std::ofstream
#include <unordered_set>   // For std::unordered_set
#include <utility>         // For std::pair
#include <filesystem>      // For std::filesystem
#include <iostream>        // For std::cout, std::endl
#include <chrono>         // For time-related functionality
#include <sstream>        // For std::stringstream
#include <stdexcept>      // For std::runtime_error
#include <algorithm>      // For std::remove_if, std::transform
#include <exception>      // For std::exception

#include "GameState.h"
#include "GameManager.h"
#include "Tank.h"
#include "Shell.h"
#include "Wall.h"
#include "Mine.h"
#include "common/ActionRequest.h"
#include "Position.h"
#include "Direction.h"
#include "RotatingAlgorithm.h"   // Include RotatingAlgorithm
#include "Logger.h"
#include "ChasingAlgorithm.h"
#include "MySatelliteView.h"
#include "MyBattleInfo.h"
#include "ActionOutcome.h"
#include "InteractiveAlgorithm.h"

using namespace std;

// Constructor implementation
GameManager::GameManager(const TankAlgorithmFactory& factory, const PlayerFactory& playerFactory, bool write_debug_file)
    : tankAlgorithmFactory(factory)
    , playerFactory(playerFactory)
    , write_debug_file(write_debug_file)
{
    LOG_INFO("Creating GameManager (no algorithms or board yet)");
    // Do not instantiate algorithms here
    LOG_INFO("GameManager constructed. Algorithms and board not yet loaded.");
}

void GameManager::readBoard(const std::string& input_file_path) {
    LOG_INFO("Reading board from input file: " + input_file_path);
    // Construct the output and debug filenames based on the input filename
    std::string output_filename, debug_filename;
    try {
        std::filesystem::path input_path(input_file_path);
        std::string input_basename = input_path.filename().string();
        output_filename = "output_" + input_basename;
        debug_filename = "debug_" + input_basename;
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR("Filesystem error processing input path '" + input_file_path + "': " + e.what());
        output_filename = "game_output_default.txt";
        debug_filename = "game_debug_default.txt";
    } catch (...) {
        LOG_ERROR("Unknown error processing input path '" + input_file_path + "'");
        output_filename = "game_output_default.txt";
        debug_filename = "game_debug_default.txt";
    }

    LOG_INFO("Opening output file: " + output_filename);
    output_file.open(output_filename);
    if (!output_file.is_open()) {
        LOG_ERROR("Failed to open output file: " + output_filename);
        // Consider if you should throw an error here if output is critical
    }
    
    if (write_debug_file) {
        LOG_INFO("Opening debug file: " + debug_filename);
        debug_file.open(debug_filename);
        if (!debug_file.is_open()) {
            LOG_ERROR("Failed to open debug file: " + debug_filename);
        }
    }
    try {
        LOG_INFO("Parsing input file: " + input_file_path);
        parseInputFile(input_file_path);
    } catch (const std::exception& e) {
        LOG_ERROR("Error parsing input file: " + std::string(e.what()));
        if (output_file.is_open()) {
            output_file << "Error parsing input file: " << e.what() << std::endl;
            output_file.close();
        }
        throw;
    }
    if (!state) {
        LOG_ERROR("Game state not initialized after parsing (unexpected error)");
        if (output_file.is_open())
            output_file.close();
        throw std::runtime_error("Game state not initialized properly");
    }
    // Use the factory to create the algorithms
    try {
        LOG_INFO("Instantiating algorithms using TankAlgorithmFactory");
        // Create algorithms for each tank
        for (const auto& [tank_id, tank] : tanks) {
            algorithms[tank_id] = tankAlgorithmFactory.create(tank->player_id, tank_id);
        }

        // Create player instances
        LOG_INFO("Creating player instances");
        player1 = playerFactory.create(1, state->board_width, state->board_height, max_game_steps, num_shells);
        player2 = playerFactory.create(2, state->board_width, state->board_height, max_game_steps, num_shells);
    } catch (const std::exception& e) {
        LOG_ERROR("Error initializing algorithms or players: " + std::string(e.what()));
        if (output_file.is_open()) output_file.close();
        throw;
    }
    LOG_INFO("GameManager board and state successfully initialized");
}

// Destructor
GameManager::~GameManager() {
    if (output_file.is_open()) {
        LOG_WARNING("GameManager destructor called while output file still open. Closing file.");
        if (!game_over) {
            output_file << "\nGame ended unexpectedly." << std::endl;
        }
        output_file.close();
    }
    if (write_debug_file && debug_file.is_open()) {
        LOG_WARNING("GameManager destructor called while debug file still open. Closing file.");
        if (!game_over) {
            debug_file << "\nGame ended unexpectedly." << std::endl;
        }
        debug_file.close();
    }
}

void GameManager::writeGameStartOutput() {
    if (!state) {
        LOG_ERROR("Game cannot run: Manager not properly initialized.");
        if (write_debug_file && debug_file.is_open()) {
            debug_file << "Error: Game Manager not properly initialized." << std::endl;
            debug_file.close();
        }
        return;
    }

    LOG_INFO("Game started");
    if (write_debug_file && debug_file.is_open()) {
        debug_file << "Game started with board size: " << state->board_width << "x" << state->board_height << " (Horizontal, Vertical)" << std::endl;
        debug_file << "Initial tank positions:" << std::endl;

        // Get all tanks for each player
        auto tanks1 = state->getTanks(1);
        auto tanks2 = state->getTanks(2);

        // Output Player 1 tanks
        for (const auto& tank : tanks1) {
            std::string tank_info = "Player 1 tank at " + tank->position.toString() +
                                   " with cannon direction: " + DirectionUtils::toString(tank->cannonDirection);
            debug_file << tank_info << std::endl;
            LOG_INFO(tank_info);
        }

        // Output Player 2 tanks
        for (const auto& tank : tanks2) {
            std::string tank_info = "Player 2 tank at " + tank->position.toString() +
                                   " with cannon direction: " + DirectionUtils::toString(tank->cannonDirection);
            debug_file << tank_info << std::endl;
            LOG_INFO(tank_info);
        }

        std::string strGameState = renderGameState(); // Initial game state rendering

        debug_file << strGameState; // Render state after all updates and cleanup
        LOG_DEBUG(strGameState);

        debug_file << "\nGame steps:" << std::endl;
    } else {
        LOG_WARNING("Debug file is not open. Game progress will not be saved.");
    }
}

void GameManager::startGameLoop() {
    // Game loop
    while (!game_over) {
        steps_count++;
        LOG_DEBUG("Processing step " + std::to_string(steps_count));
        if (write_debug_file && debug_file.is_open()) {
            debug_file << "\nStep " << steps_count << ":" << std::endl;
        }
        processStep(); // Process tanks, shells, collisions for one step

        if (write_debug_file && debug_file.is_open()) {
            debug_file << renderGameState(); // Render game state after each step
        }

        // Check game over conditions after processing the step
        if (checkGameOver()) {
            game_over = true; // Set flag to exit loop
            LOG_INFO("Game over detected at step " + std::to_string(steps_count));
        }

        // Check maximum step limit
        if (steps_count >= max_game_steps) {
            LOG_WARNING("Maximum steps (" + std::to_string(max_game_steps) +") reached. Ending game as a tie.");
            game_over = true;
            winner = "Tie";
            game_over_reason = "Maximum steps reached.";
        }
    }
}

// Main game loop implementation
void GameManager::run() {
    writeGameStartOutput();
    
    if (!game_over) {
        startGameLoop();
    }
    LOG_INFO("Game ended after " + std::to_string(steps_count) + " steps");
    writeGameResults(); // Write final results and close file
}


// Parse the input file to set up the game state
void GameManager::parseInputFile(const std::string& file_path) {
    LOG_INFO("Opening input file: " + file_path);
    std::ifstream input_file(file_path);
    if (!input_file.is_open()) {
        std::string error_msg = "Failed to open input file: " + file_path;
        LOG_ERROR(error_msg);
        throw std::runtime_error(error_msg);
    }

    std::vector<std::string> warnings;
    std::string line;

    // Read map name/description (Line 1)
    if (!std::getline(input_file, map_name)) {
        throw std::runtime_error("Empty map file");
    }
    LOG_INFO("Map name: " + map_name);

    // Parse configuration parameters
    bool found_max_steps = false;
    bool found_num_shells = false;
    bool found_rows = false;
    bool found_cols = false;

    // Read next 4 lines for parameters
    for (int i = 0; i < 4; i++) {
        if (!std::getline(input_file, line)) {
            throw std::runtime_error("Missing required configuration parameters");
        }

        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Split on = and parse value
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            throw std::runtime_error("Invalid parameter format: " + line);
        }

        std::string param = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Trim whitespace from param and value
        param.erase(0, param.find_first_not_of(" \t\r\n"));
        param.erase(param.find_last_not_of(" \t\r\n") + 1);
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);

        try {
            if (i==0 && param == "MaxSteps") {
                max_game_steps = std::stoi(value);
                found_max_steps = true;
            } else if (i==1 && param == "NumShells") {
                num_shells = std::stoi(value);
                Tank::initial_shells = num_shells;
                found_num_shells = true;
            } else if (i==2 && param == "Rows") {
                num_rows = std::stoi(value);
                found_rows = true;
            } else if (i==3 && param == "Cols") {
                num_cols = std::stoi(value);
                found_cols = true;
            }
        } catch (const std::exception& e) {
            throw std::runtime_error("Invalid value in parameter: " + line);
        }
    }

    // Validate all parameters were found
    if (!found_max_steps || !found_num_shells || !found_rows || !found_cols) {
        throw std::runtime_error("Missing required parameters");
    }

    // Validate parameter values
    if (max_game_steps <= 0 || num_shells <= 0 || num_rows <= 0 || num_cols <= 0) {
        throw std::runtime_error("Invalid parameter values: all must be positive");
    }

    // Create game state with parsed dimensions
    state = std::make_shared<GameState>(num_cols, num_rows);
    LOG_INFO("Created game state with dimensions: " + std::to_string(num_cols) + "x" + std::to_string(num_rows));

    // Parse board content
    std::vector<std::string> board_lines;
    int actual_height_read = 0;

    // Now read exactly num_rows lines, regardless of content
    while (std::getline(input_file, line) && actual_height_read < num_rows) {
        // Trim whitespace at ends
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Handle line length
        if (static_cast<int>(line.length()) > num_cols) {
            line = line.substr(0, num_cols);
            warnings.push_back("Line " + std::to_string(actual_height_read + 1) + " truncated.");
        } else if (static_cast<int>(line.length()) < num_cols) {
            line.append(num_cols - line.length(), ' ');
            warnings.push_back("Line " + std::to_string(actual_height_read + 1) + " padded.");
        }

        board_lines.push_back(line);
        actual_height_read++;
    }

    // Pad with empty lines if needed
    while (actual_height_read < num_rows) {
        board_lines.push_back(std::string(num_cols, ' '));
        actual_height_read++;
        warnings.push_back("Added empty line " + std::to_string(actual_height_read));
    }

    // Populate game state objects
    int player1_tanks = 0, player2_tanks = 0;
    for (int y = 0; y < num_rows; ++y) {
        for (int x = 0; x < num_cols; ++x) {
            Position pos(x, y);
            char c = board_lines[y][x];

            switch (c) {
                case '1': {
                    ++player1_tanks;
                    auto tank = std::make_shared<Tank>(pos, 1, Direction::LEFT);
                    state->addObject(tank);
                    tanks[tank->getTankId()] = tank;
                    break;
                }
                case '2': {
                    ++player2_tanks;
                    auto tank = std::make_shared<Tank>(pos, 2, Direction::RIGHT);
                    state->addObject(tank);
                    tanks[tank->getTankId()] = tank;
                    break;
                }
                case '#':
                    state->addObject(std::make_shared<Wall>(pos));
                    break;
                case '@':
                    state->addObject(std::make_shared<Mine>(pos));
                    break;
                case ' ':
                default:
                    break;
            }
        }
    }

    // Check for immediate win conditions based on initial tank counts
    if (player1_tanks == 0 && player2_tanks == 0) {
        game_over = true;
        winner = "Tie";
        game_over_reason = "No tanks found for either player.";
    } else if (player1_tanks == 0) {
        game_over = true;
        winner = "Player 2";
        game_over_reason = "No tanks found for player 1.";
    } else if (player2_tanks == 0) {
        game_over = true;
        winner = "Player 1";
        game_over_reason = "No tanks found for player 2.";
    }

    LOG_INFO("Successfully parsed board with " + std::to_string(player1_tanks) + " P1 tanks and " + 
             std::to_string(player2_tanks) + " P2 tanks");
    checkInputWarnings(file_path, warnings);
}

/**
 * @brief Initializes player data structures and performs start-of-step actions.
 */
void GameManager::prepareStep(std::unordered_map<int, TankStepData>& tank_data) {
    // Decrement cooldowns for all tanks
    for (const auto& [tank_id, tank] : tanks) {
        if (tank) {
            tank->decrementCooldown();
        }
    }

    // Initialize tank data for this step
    for (const auto& [tank_id, tank] : tanks) {
        if (tank) {
            TankStepData data;
            data.tank = tank;
            data.intended_position = tank->position;
            
            // Initialize logged action
            data.logged_action.player_id = tank->player_id;
            data.logged_action.tank_id = tank_id;
            data.logged_action.tank = tank;
            
            // If tank was already destroyed before this step
            if (tank->is_destroyed) {
                data.logged_action.was_tank_destroyed = true;
                data.logged_action.killed_this_step = false;  // Tank was already dead
                data.logged_action.action = ActionRequest::DoNothing;  // No action for dead tanks
            }
            
            tank_data[tank_id] = data;
        }
    }

    // Check if any algorithm is interactive and print board if needed
    bool has_interactive = false;
    for (const auto& [tank_id, algo] : algorithms) {
        if (dynamic_cast<InteractiveAlgorithm*>(algo.get())) {
            has_interactive = true;
            break;
        }
    }

    if (has_interactive) {
        std::cout << "\n--- Step " << steps_count << " ---" << std::endl;
        std::cout << renderGameState() << std::endl;
        for (const auto& [tank_id, data] : tank_data) {
            if (data.tank) {
                std::cout << "Player " << data.tank->player_id << " Tank " << tank_id << ": Pos" << data.tank->position
                         << " Dir:" << DirectionUtils::toString(data.tank->cannonDirection)
                         << " Shells:" << data.tank->shells_remaining
                         << " CD:" << data.tank->cooldown_remaining << std::endl;
            }
        }
    }
}

/**
 * @brief Gets actions from algorithms for active tanks.
 */
void GameManager::getPlayerActions(std::unordered_map<int, TankStepData>& tank_data) {
    for (auto& [tank_id, data] : tank_data) {
        if (data.tank && !data.tank->is_destroyed) {
            auto algorithm_it = algorithms.find(tank_id);
            if (algorithm_it != algorithms.end()) {
                data.action = algorithm_it->second->getAction();
                LOG_DEBUG("GameManager: Tank " + std::to_string(tank_id) + " Algo chose: " + actionToString(data.action));
            } else {
                data.action = ActionRequest::DoNothing;
            }
        } else {
            data.action = ActionRequest::DoNothing;
        }
    }
}

/**
 * @brief Calls transitionMovementState for active tanks to determine action outcomes.
 */
void GameManager::processTankTransitions(std::unordered_map<int, TankStepData>& tank_data) {
    for (auto& [tank_id, data] : tank_data) {
        if (data.tank && !data.tank->is_destroyed) {
            data.outcome = data.tank->transitionMovementState(data.action);
            LOG_DEBUG("GameManager: Tank " + std::to_string(tank_id) + " Outcome: " + outcomeToString(data.outcome));
        } else {
            data.outcome = ActionOutcome::NONE;
        }
    }
}

/**
 * @brief Executes actions with immediate effects (shooting) and prepares logging.
 */
void GameManager::executeImmediateActions(std::unordered_map<int, TankStepData>& tank_data) {
    // Create board_matrix for satellite view
    std::vector<std::vector<char>> board_matrix(state->board_width, std::vector<char>(state->board_height, ' '));
    for (const auto& obj : state->objects) {
        if (!obj || obj->is_destroyed) continue;
        board_matrix[obj->position.x][obj->position.y] = obj->render();
    }
    // Making sure Shells obstruct Mines, by overwriting the matrix cells
    for (const auto& obj : state->objects) {
        if (!obj || obj->is_destroyed) continue;
        if (obj->type == GameObjectType::SHELL)
            board_matrix[obj->position.x][obj->position.y] = obj->render();
    }

    for (auto& [tank_id, data] : tank_data) {
        if (!data.tank) continue;

        if (data.outcome == ActionOutcome::SHOT_INITIATED) {
            std::shared_ptr<Shell> newShell = data.tank->shoot();
            if (newShell) {
                state->addObject(newShell);
                data.logged_action = LoggedAction{data.tank->player_id, tank_id, ActionRequest::Shoot, false, data.tank->is_destroyed, false, data.tank};
            } else {
                data.logged_action = LoggedAction{data.tank->player_id, tank_id, ActionRequest::Shoot, true, data.tank->is_destroyed, false, data.tank}; // Failed shot
            }
        } else if (data.action == ActionRequest::Shoot && data.outcome == ActionOutcome::INVALID_ACTION) {
            data.logged_action = LoggedAction{data.tank->player_id, tank_id, ActionRequest::Shoot, true, data.tank->is_destroyed, false, data.tank}; // Invalid shot attempt
        } else if (data.outcome == ActionOutcome::ROTATED) {
            data.logged_action = LoggedAction{data.tank->player_id, tank_id, data.action, false, data.tank->is_destroyed, false, data.tank}; // Successful rotation
        } else if (data.outcome == ActionOutcome::INVALID_ACTION) {
            data.logged_action = LoggedAction{data.tank->player_id, tank_id, data.action, true, data.tank->is_destroyed, false, data.tank}; // Other invalid actions
        } else if (data.outcome == ActionOutcome::MOVE_PENDING || data.outcome == ActionOutcome::STATE_CHANGED) {
            data.logged_action = LoggedAction{data.tank->player_id, tank_id, data.action, false, data.tank->is_destroyed, false, data.tank}; // Movement or state change
        } else if (data.outcome == ActionOutcome::RETURNING_BATTLE_INFO) {
            auto algorithm_it = algorithms.find(tank_id);
            board_matrix[data.tank->position.x][data.tank->position.y] = '%';
            auto satelliteView = std::make_shared<MySatelliteView>(board_matrix);
            if (algorithm_it != algorithms.end()) {
                if (data.tank->player_id == 1) {
                    player1->updateTankWithBattleInfo(*algorithm_it->second, *satelliteView);
                } else {
                    player2->updateTankWithBattleInfo(*algorithm_it->second, *satelliteView);
                }
            }
            board_matrix[data.tank->position.x][data.tank->position.y] = data.tank->render();
            data.logged_action = LoggedAction{data.tank->player_id, tank_id, ActionRequest::GetBattleInfo, false, data.tank->is_destroyed, false, data.tank};
        }
    }
}

/**
 * @brief Calculates intended positions for tanks based on their movement progress for one sub-step.
 */
void GameManager::calculateIntendedTankPositionsSubStep(std::unordered_map<int, TankStepData>& tank_data, int max_speed) {
    // Reset intended positions to current before calculation
    for (auto& [tank_id, data] : tank_data) {
        if (data.tank) data.intended_position = data.tank->position;
    }

    for (auto& [tank_id, data] : tank_data) {
        if (data.tank && data.tank->move_intent_this_step) {
            data.intended_position = data.tank->updateMovementProgress(max_speed, state->board_width, state->board_height);
            if (data.intended_position != data.tank->position) {
                LOG_DEBUG("GameManager: Tank " + std::to_string(tank_id) + " intends to move to " + data.intended_position.toString());
            }
        }
    }
}

/**
 * @brief Calculates intended positions for all active shells for one sub-step.
 */
std::vector<std::pair<std::shared_ptr<Shell>, Position>> GameManager::calculateShellIntendedPositionsSubStep(int max_speed) {
    std::vector<std::pair<std::shared_ptr<Shell>, Position>> intendedShellPositions;
    auto current_shells = state->getShells(); // Get active shells

    for (auto& shell : current_shells) {
        // No need to check is_destroyed here, getShells only returns active ones
        Position currentPos = shell->position;
        Position intendedPos = currentPos;

        shell->movement_progress += shell->speed;
        if (shell->movement_progress >= max_speed) {
            Position offset = DirectionUtils::getMovementOffset(shell->direction);
            intendedPos.x = (currentPos.x + offset.x + state->board_width) % state->board_width;
            intendedPos.y = (currentPos.y + offset.y + state->board_height) % state->board_height;
            shell->movement_progress -= max_speed;
            LOG_DEBUG("Shell at " + currentPos.toString() + " intends move to " + intendedPos.toString());
        }
        intendedShellPositions.emplace_back(shell, intendedPos);
    }
    return intendedShellPositions;
}

/**
 * @brief Updates the actual positions of tanks and shells after collisions are resolved.
 */
void GameManager::updateObjectPositionsSubStep(
        std::unordered_map<int, TankStepData>& tank_data,
        const std::vector<std::pair<std::shared_ptr<Shell>, Position>>& shells_intended_positions)
{
    // Update tank positions
    for (auto& [tank_id, data] : tank_data) {
        if (data.tank && !data.tank->is_destroyed && !data.blocked_this_sub_step) {
            if (data.tank->position != data.intended_position) {
                LOG_DEBUG("Tank " + std::to_string(tank_id) + " moved to " + data.intended_position.toString());
                // Log movement action here (successful move)
                data.logged_action = LoggedAction{data.tank->player_id, 
                    tank_id,
                    (data.tank->multiplier > 0 ? ActionRequest::MoveForward : ActionRequest::MoveBackward), 
                    false,
                    data.tank->is_destroyed,
                    false,
                    data.tank};
            }
            data.tank->position = data.intended_position;
        } else if (data.tank && data.blocked_this_sub_step) {
            // Log failed movement action if it hasn't been logged already
            if (data.logged_action.player_id == 0 || data.logged_action.action != data.action) {
                data.logged_action = LoggedAction{data.tank->player_id, tank_id, data.action, true, data.tank->is_destroyed, false, data.tank}; // Log blocked move
            }
        }
    }

    // Update shell positions (only for those not destroyed)
    for (const auto& [shell, intended_pos] : shells_intended_positions) {
        if (shell && !shell->is_destroyed) {
            if (shell->position != intended_pos) {
                LOG_DEBUG("Shell moved from " + shell->position.toString() + " to " + intended_pos.toString());
            }
            shell->position = intended_pos;
        }
    }
}

// --- Collision Resolution Helpers ---

void GameManager::resolveShellWallCollisions(
        std::vector<std::pair<std::shared_ptr<Shell>, Position>>& shells_intended_positions,
        const std::unordered_set<Position>& wall_positions)
{
    for (auto& [shell, pos] : shells_intended_positions) {
        if (shell && !shell->is_destroyed && wall_positions.count(pos)) {
            auto wall = state->getWallAt(pos); // Assumes getWallAt handles destroyed walls correctly or returns nullptr
            if (wall && !wall->is_destroyed) { // Check wall exists and is active
                bool wall_destroyed_by_this_shell = wall->takeDamage();
                LOG_INFO("Wall at " + pos.toString() + " hit by shell. Health: " + std::to_string(wall->health));
                if (wall_destroyed_by_this_shell) LOG_INFO("Wall at " + pos.toString() + " destroyed.");
                shell->is_destroyed = true; // Destroy the shell
                LOG_INFO("Shell destroyed hitting wall at " + pos.toString());
            }
        }
    }
}

void GameManager::resolveTankWallCollisions(
        std::unordered_map<int, TankStepData>& tank_data,
        const std::unordered_set<Position>& wall_positions)
{
    for (auto& [tank_id, data] : tank_data) {
        if (data.tank && data.tank->move_intent_this_step &&
            data.intended_position != data.tank->position &&
            wall_positions.count(data.intended_position))
        {
            LOG_DEBUG("Tank " + std::to_string(tank_id) + " movement to " + data.intended_position.toString() + " blocked by wall.");
            data.blocked_this_sub_step = true;
            data.intended_position = data.tank->position; // Reset intended position
            data.tank->movement_progress = 0; // Stop progress
            data.tank->move_intent_this_step = false; // Cancel intent for *this* step
        }
    }
}

void GameManager::resolveShellShellCollisions(
        std::vector<std::pair<std::shared_ptr<Shell>, Position>>& shells_intended_positions)
{
    std::unordered_map<Position, std::vector<std::shared_ptr<Shell>>> shells_at_intended_pos;
    for (const auto& [shell, pos] : shells_intended_positions) {
        if (shell && !shell->is_destroyed) {
            shells_at_intended_pos[pos].push_back(shell);
        }
    }

    for (const auto& [pos, shells] : shells_at_intended_pos) {
        if (shells.size() > 1) {
            LOG_INFO("Shell-shell collision at " + pos.toString());
            for (auto& shell : shells) {
                shell->is_destroyed = true;
                LOG_INFO(" Shell involved.");
            }
        }
    }
    // Check for head-on collisions between existing shells and moving shells
    for(auto& shell : state->getShells()) {
        auto pos = shell->position;
        if (!shell->is_destroyed) {
            for (const auto& [otherShell, intendedPos] : shells_intended_positions) {
                if (otherShell && !otherShell->is_destroyed && // Different shells
                    intendedPos == pos &&
                        shell != otherShell &&// Moving shell intends to land on current shell's spot
                    std::abs(static_cast<int>(shell->direction) - static_cast<int>(otherShell->direction)) == 4) // Opposite directions
                {
                    LOG_INFO("Head-on shell collision at " + pos.toString());                    shell->is_destroyed = true;
                    otherShell->is_destroyed = true;
                }
            }
        }
    }
}

void GameManager::resolveShellTankCollisions(
        std::unordered_map<int, TankStepData>& tank_data,
        std::vector<std::pair<std::shared_ptr<Shell>, Position>>& shells_intended_positions)
{
    // Use the potentially updated intended positions if tanks were blocked
    std::unordered_map<int, Position> tank_effective_positions;
    std::unordered_map<int, Position> tank_current_positions;
    for (const auto& [tank_id, data] : tank_data) {
        if (data.tank && !data.tank->is_destroyed) {
            tank_effective_positions[tank_id] = data.blocked_this_sub_step ? data.tank->position : data.intended_position;
            tank_current_positions[tank_id] = data.tank->position;
        }
    }

    std::vector<std::shared_ptr<Shell>> shells_hitting_tanks;

    // Check shells hitting tanks
    for (auto& [shell, intended_pos] : shells_intended_positions) {
        if (!shell || shell->is_destroyed) continue;

        Position current_shell_pos = shell->position;

        for (const auto& [tank_id, effective_pos] : tank_effective_positions) {
            Position current_tank_pos = tank_current_positions[tank_id];

            // Case 1: Shell's intended position hits tank's effective position
            bool direct_hit = (intended_pos == effective_pos);

            // Case 2: Shell and tank pass through each other (swap positions)
            bool pass_through = (intended_pos == current_tank_pos && current_shell_pos == effective_pos);

            if (direct_hit || pass_through) {
                auto& data = tank_data[tank_id];
                if (!data.tank->is_destroyed) {  // Only set killed_this_step if tank wasn't already destroyed
                    data.tank->is_destroyed = true;
                    data.logged_action.was_tank_destroyed = true;
                    data.logged_action.killed_this_step = true;  // Set this flag for tanks killed this step
                }
                shells_hitting_tanks.push_back(shell);
                if (direct_hit) {
                    LOG_INFO("Tank " + std::to_string(tank_id) + " at " + effective_pos.toString() + " hit by shell!");
                } else {
                    LOG_INFO("Tank " + std::to_string(tank_id) + " passed through shell at " + current_shell_pos.toString() + "!");
                }
                break; // One shell can only hit one tank
            }
        }
    }

    // Destroy shells that hit tanks
    for (const auto& shell : shells_hitting_tanks) {
        shell->is_destroyed = true;
    }
}

void GameManager::resolveTankMineCollisions(
        std::unordered_map<int, TankStepData>& tank_data,
        const std::unordered_set<Position>& mine_positions)
{
    // Use effective positions (considering wall blocks)
    std::unordered_map<int, Position> tank_effective_positions;
    for (const auto& [tank_id, data] : tank_data) {
        if (data.tank && !data.tank->is_destroyed) {
            tank_effective_positions[tank_id] = data.blocked_this_sub_step ? data.tank->position : data.intended_position;
        }
    }

    for (const auto& [tank_id, effective_pos] : tank_effective_positions) {
        if (mine_positions.count(effective_pos)) {
            auto mine = state->getMineAt(effective_pos);
            if (mine && !mine->is_destroyed) {
                auto& data = tank_data[tank_id];
                if (!data.tank->is_destroyed) {  // Only set killed_this_step if tank wasn't already destroyed
                    data.tank->is_destroyed = true;
                    data.logged_action.was_tank_destroyed = true;
                    data.logged_action.killed_this_step = true;  // Set this flag for tanks killed this step
                }
                mine->is_destroyed = true;
                LOG_INFO("Tank " + std::to_string(tank_id) + " stepped on mine at " + effective_pos.toString());
            }
        }
    }
}

void GameManager::resolveTankTankCollisions(std::unordered_map<int, TankStepData>& tank_data) {
    std::vector<std::pair<int, int>> colliding_tank_pairs;

    // Check for collisions between all pairs of tanks
    std::vector<int> tank_ids;
    for (const auto& [tank_id, data] : tank_data) {
        if (data.tank && !data.tank->is_destroyed) {
            tank_ids.push_back(tank_id);
        }
    }

    for (size_t i = 0; i < tank_ids.size(); ++i) {
        for (size_t j = i + 1; j < tank_ids.size(); ++j) {
            int tank1_id = tank_ids[i];
            int tank2_id = tank_ids[j];

            auto& data1 = tank_data[tank1_id];
            auto& data2 = tank_data[tank2_id];

            Position tank1_effective_pos = data1.blocked_this_sub_step ? data1.tank->position : data1.intended_position;
            Position tank2_effective_pos = data2.blocked_this_sub_step ? data2.tank->position : data2.intended_position;

            if (tank1_effective_pos == tank2_effective_pos) {
                // Check if they actually moved or started in the same spot
                bool tank1_moved = !data1.blocked_this_sub_step && data1.intended_position != data1.tank->position;
                bool tank2_moved = !data2.blocked_this_sub_step && data2.intended_position != data2.tank->position;

                // Collision only happens if at least one tank moved into the spot,
                // or if they started in the same spot (input error).
                if (tank1_moved || tank2_moved || data1.tank->position == data2.tank->position) {
                    colliding_tank_pairs.push_back({tank1_id, tank2_id});
                    LOG_INFO("Tanks " + std::to_string(tank1_id) + " and " + std::to_string(tank2_id) + 
                            " collided at " + tank1_effective_pos.toString());
                } else {
                    LOG_DEBUG("Tanks intended same spot but neither moved, no collision: " + tank1_effective_pos.toString());
                }
            }
        }
    }

    // Destroy colliding tanks
    for (const auto& [tank1_id, tank2_id] : colliding_tank_pairs) {
        auto& data1 = tank_data[tank1_id];
        auto& data2 = tank_data[tank2_id];
        
        if (!data1.tank->is_destroyed) {  // Only set killed_this_step if tank wasn't already destroyed
            data1.tank->is_destroyed = true;
            data1.logged_action.was_tank_destroyed = true;
            data1.logged_action.killed_this_step = true;
        }
        
        if (!data2.tank->is_destroyed) {  // Only set killed_this_step if tank wasn't already destroyed
            data2.tank->is_destroyed = true;
            data2.logged_action.was_tank_destroyed = true;
            data2.logged_action.killed_this_step = true;
        }
    }
}

/**
 * @brief Resolves all types of collisions for one sub-step based on intended positions.
 * Updates the is_destroyed flag on objects and the blocked_this_sub_step flag for tanks.
 */
void GameManager::resolveCollisionsSubStep(
        std::unordered_map<int, TankStepData>& tank_data,
        std::vector<std::pair<std::shared_ptr<Shell>, Position>>& shells_intended_positions)
{
    // Get current wall and mine positions (needed for multiple checks)
    std::unordered_set<Position> wall_positions;
    std::unordered_set<Position> mine_positions;
    for(const auto& obj : state->objects) {
        if(obj && !obj->is_destroyed) {
            if(obj->type == GameObjectType::WALL) {
                wall_positions.insert(obj->position);
            } else if (obj->type == GameObjectType::MINE) {
                mine_positions.insert(obj->position);
            }
        }
    }

    // Resolve collisions in order of precedence:
    // 1. Shells hitting walls
    resolveShellWallCollisions(shells_intended_positions, wall_positions);

    // 2. Tanks hitting Walls (Prevents movement)
    resolveTankWallCollisions(tank_data, wall_positions);

    // 3. Shell-Shell Collision
    resolveShellShellCollisions(shells_intended_positions);

    // 4. Shells hitting Tanks (Considers if tanks were blocked by walls)
    resolveShellTankCollisions(tank_data, shells_intended_positions);

    // 5. Tanks hitting Mines (Considers if tanks were blocked by walls)
    resolveTankMineCollisions(tank_data, mine_positions);

    // 6. Tank-Tank Collision (Considers if tanks were blocked by walls)
    resolveTankTankCollisions(tank_data);
}

/**
 * @brief Runs the sub-step loop for movement and collision detection/resolution.
 */
void GameManager::runMovementAndCollisionSubSteps(std::unordered_map<int, TankStepData>& tank_data) {
    int max_speed = 0;
    for (const auto& [tank_id, data] : tank_data) {
        if (data.tank) max_speed = std::max(max_speed, Tank::speed);
    }
    // Check if there are any shells to determine max speed accurately
    bool shells_exist = false;
    for (const auto& obj : state->objects) {
        if (obj && obj->type == GameObjectType::SHELL && !obj->is_destroyed) {
            shells_exist = true;
            break;
        }
    }
    if (shells_exist) {
        max_speed = std::max(max_speed, Shell::speed);
    }
    if (max_speed == 0) return; // No moving objects

    for (int s = 0; s < max_speed; ++s) {
        LOG_DEBUG("GameManager: Sub-step " + std::to_string(s+1) + "/" + std::to_string(max_speed));

        // Reset blocked flags for this sub-step
        for (auto& [tank_id, data] : tank_data) {
            data.blocked_this_sub_step = false;
        }

        // 1. Calculate intended positions for tanks and shells for this sub-step
        calculateIntendedTankPositionsSubStep(tank_data, max_speed);
        auto shells_intended_positions = calculateShellIntendedPositionsSubStep(max_speed);

        // 2. Resolve all collisions based on intended positions
        resolveCollisionsSubStep(tank_data, shells_intended_positions);

        // 3. Update actual object positions based on resolved collisions and movement intent
        updateObjectPositionsSubStep(tank_data, shells_intended_positions);

        // 4. Early exit check: If all tanks are destroyed, can break sub-step loop
        bool any_tank_alive = false;
        for (const auto& [tank_id, data] : tank_data) {
            if (data.tank && !data.tank->is_destroyed) {
                any_tank_alive = true;
                break;
            }
        }
        if (!any_tank_alive) {
            LOG_DEBUG("All tanks destroyed during sub-step, breaking loop.");
            break;
        }
    }
}


/**
 * @brief Checks if all shells are used and updates game end timer if necessary.
 */
void GameManager::checkShellDepletionStatus() {
    if (!all_shells_used) {
        auto final_tank1 = state->getTank(1); // Re-fetch in case destroyed
        auto final_tank2 = state->getTank(2);
        bool p1_has_shells = final_tank1 && final_tank1->shells_remaining > 0;
        bool p2_has_shells = final_tank2 && final_tank2->shells_remaining > 0;
        bool shells_on_board = false;
        for(const auto& shell : state->getShells()){ if(shell && !shell->is_destroyed) { shells_on_board = true; break;} }

        if (!p1_has_shells && !p2_has_shells && !shells_on_board) {
            all_shells_used = true;
            steps_after_no_shells = 0; // Start counter
            std::string msg = "All shells have been used. Game will end in " +
                              std::to_string(MAX_STEPS_AFTER_NO_SHELLS) + " steps if no winner.";
            LOG_INFO(msg);
        }
    } else if (!game_over) { // Only increment if game isn't already over
        steps_after_no_shells++;
        LOG_DEBUG("Steps after no shells: " + std::to_string(steps_after_no_shells));
        if (steps_after_no_shells >= MAX_STEPS_AFTER_NO_SHELLS) {
            LOG_INFO("Max steps reached after all shells used. Ending game as Tie.");
            game_over = true; // Set game over flag
            winner = "Tie";
            game_over_reason = "Maximum steps (" + std::to_string(MAX_STEPS_AFTER_NO_SHELLS) +
                               ") reached after all shells were used.";
        }
    }
}

/**
 * @brief Performs end-of-step cleanup and final game state checks.
 */
void GameManager::finalizeStep() {
    cleanupDestroyedObjects();

    // Final game over check based on tank status *after* cleanup
    if (!game_over) { // Only check if not already ended by shell depletion/max steps
        checkGameOver(); // This will set game_over, winner, reason if applicable
    }
}

/**
 * @brief Processes a single step (turn) of the game. Orchestrates helper methods.
 */
void GameManager::processStep() {
    std::unordered_map<int, TankStepData> tank_data;

    // 1. Prepare step data and perform initial actions (like cooldowns)
    prepareStep(tank_data);

    // 2. Get actions from player algorithms
    getPlayerActions(tank_data);

    // 3. Process tank state transitions based on actions
    processTankTransitions(tank_data);

    // 4. Execute immediate actions like shooting
    executeImmediateActions(tank_data);

    // 5. Run movement and collision sub-steps
    runMovementAndCollisionSubSteps(tank_data);

    // 6. Log actions accumulated during the step
    for (auto& [tank_id, data] : tank_data) {
        // Update was_tank_destroyed flag based on tank's current state
        if (data.tank && data.tank->is_destroyed) {
            data.logged_action.was_tank_destroyed = true;
        }
        logAction(data.logged_action);
    }

    // 7. Check shell depletion status and potentially end game
    checkShellDepletionStatus();

    // 8. Finalize step: cleanup, check game over, render state
    finalizeStep(); // Includes cleanup, final game over check, and rendering
}

// Cleanup destroyed objects from game state
void GameManager::cleanupDestroyedObjects() {
    // Use erase-remove idiom for efficient removal from vector
    auto new_end = std::remove_if(state->objects.begin(), state->objects.end(),
                                  [](const std::shared_ptr<GameObject>& obj) {
                                      // Remove if object is null or marked as destroyed
                                      return !obj || obj->is_destroyed;
                                  });

    // Calculate how many objects were removed (optional)
    size_t removed_count = std::distance(new_end, state->objects.end());
    if (removed_count > 0) {
        LOG_DEBUG("Cleaning up " + std::to_string(removed_count) + " destroyed objects.");
    }

    // Erase the removed elements from the vector
    state->objects.erase(new_end, state->objects.end());
}


// Log action to action_log and debug file
void GameManager::logAction(const LoggedAction& logged_action) {
    // Avoid logging if player_id is 0 (default/uninitialized)
    if (logged_action.player_id == 0) {
        return;
    }

    // Format the action string
    std::string action_str;
    if (logged_action.was_tank_destroyed) {
        if (logged_action.killed_this_step) {
            // Tank was killed this step - show its last action with (killed)
            action_str = actionToString(logged_action.action);
            if (logged_action.is_bad) {
                action_str += " (ignored)";
            }
            action_str += " (killed)";
        } else {
            // Tank was already dead - just show "killed"
            action_str = "killed";
        }
    } else {
        // Normal action logging for alive tanks
        action_str = actionToString(logged_action.action);
        if (logged_action.is_bad) {
            action_str += " (ignored)";
        }
    }

    // Log to debug file if open and enabled
    if (write_debug_file && debug_file.is_open()) {
        debug_file << "Player " << logged_action.player_id << ": " << action_str << std::endl;
    }

    // Log to internal log vector
    action_log.push_back(logged_action);
    LOG_DEBUG("ActionRequest logged: Player " + std::to_string(logged_action.player_id) + ": " + action_str);
}

// Check for game over conditions based on final tank states
// This function is called AFTER the step's collisions are resolved.
bool GameManager::checkGameOver() {
    // Game over flag might already be set by max steps logic or initial tank check
    if (game_over) {
        return true;
    }

    // Count alive tanks for each player
    int tanks1_alive = 0;
    int tanks2_alive = 0;
    
    for (const auto& [tank_id, tank] : tanks) {
        if (tank && !tank->is_destroyed) {
            if (tank->player_id == 1) tanks1_alive++;
            else if (tank->player_id == 2) tanks2_alive++;
        }
    }

    // Scenario 1: Player 1 wins (Tank 1 alive, Tank 2 not)
    if (tanks1_alive > 0 && tanks2_alive == 0) {
        winner = "Player 1";
        game_over_reason = "Player 1 won with " + std::to_string(tanks1_alive) + " tanks still alive";
        LOG_INFO("Game Over: Player 1 wins. " + game_over_reason);
        game_over = true;
        return true;
    }
    // Scenario 2: Player 2 wins (Tank 2 alive, Tank 1 not)
    if (tanks2_alive > 0 && tanks1_alive == 0) {
        winner = "Player 2";
        game_over_reason = "Player 2 won with " + std::to_string(tanks2_alive) + " tanks still alive";
        LOG_INFO("Game Over: Player 2 wins. " + game_over_reason);
        game_over = true;
        return true;
    }
    // Scenario 3: Tie (Both players have no tanks)
    if (tanks1_alive == 0 && tanks2_alive == 0) {
        winner = "Tie";
        game_over_reason = "Both players have no tanks";
        LOG_INFO("Game Over: Tie. " + game_over_reason);
        game_over = true;
        return true;
    }

    return false;
}


// Write game results to output and debug files
void GameManager::writeGameResults() {
    // Write detailed results to debug file
    if (write_debug_file && debug_file.is_open()) {
        std::string result = "Game over after " + std::to_string(steps_count) + " steps.";
        std::string winner_info = "Result: " + getGameResultString();
        std::string reason = "Reason: " + (game_over_reason.empty() ? "Game ended." : game_over_reason);

        LOG_INFO(result);
        LOG_INFO(winner_info);
        LOG_INFO(reason);

        debug_file << "\n" << "====================" << std::endl;
        debug_file << result << std::endl;
        debug_file << winner_info << std::endl;
        debug_file << reason << std::endl;
        debug_file << "====================" << std::endl;

        debug_file << "\nFinal game state:" << std::endl;
        debug_file << renderGameState() << std::endl; // Render final board state

        LOG_DEBUG("Closing debug file");
        debug_file.close();
    }

    // Write action summary to output file
    if (output_file.is_open()) {
        // Get the number of tanks in the game
        size_t num_tanks = tanks.size();

        // Write all actions in groups based on number of tanks
        for (size_t i = 0; i < action_log.size(); i += num_tanks) {
            // Get actions for this step and sort them by tank_id
            std::vector<LoggedAction> step_actions;
            for (size_t j = 0; j < num_tanks && i + j < action_log.size(); ++j) {
                step_actions.push_back(action_log[i + j]);
            }
            
            // Sort by tank_id to maintain tank sequence order (top-left to bottom-right)
            std::sort(step_actions.begin(), step_actions.end(),
                [](const LoggedAction& a, const LoggedAction& b) {
                    return a.tank_id < b.tank_id;
                });

            // Write actions for this step
            for (size_t j = 0; j < step_actions.size(); ++j) {
                if (j > 0) output_file << ", ";
                
                // Format the action string
                std::string action_str;
                if (step_actions[j].was_tank_destroyed) {
                    if (step_actions[j].killed_this_step) {
                        // Tank was killed this step - show its action with (killed)
                        action_str = actionToString(step_actions[j].action);
                        if (step_actions[j].is_bad) {
                            action_str += " (ignored)";
                        }
                        action_str += " (killed)";
                    } else {
                        // Tank was already dead - just show "killed"
                        action_str = "killed";
                    }
                } else {
                    // Normal action logging for alive tanks
                    action_str = actionToString(step_actions[j].action);
                    if (step_actions[j].is_bad) {
                        action_str += " (ignored)";
                    }
                }
                output_file << action_str;
            }
            output_file << std::endl;
        }

        // Write final result line according to the specified format
        auto tanks1 = state->getTanks(1);
        auto tanks2 = state->getTanks(2);
        int tanks1_alive = std::count_if(tanks1.begin(), tanks1.end(), 
            [](const auto& tank) { return !tank->is_destroyed; });
        int tanks2_alive = std::count_if(tanks2.begin(), tanks2.end(), 
            [](const auto& tank) { return !tank->is_destroyed; });

        if (winner == "Player 1" || winner == "Player 2") {
            int winner_num = (winner == "Player 1") ? 1 : 2;
            int tanks_alive = (winner == "Player 1") ? tanks1_alive : tanks2_alive;
            output_file << "Player " << winner_num << " won with " << tanks_alive << " tanks still alive" << std::endl;
        } else if (steps_count >= max_game_steps) {
            output_file << "Tie, reached max steps = " << max_game_steps 
                       << ", player 1 has " << tanks1_alive << " tanks, player 2 has " 
                       << tanks2_alive << " tanks" << std::endl;
        } else if (all_shells_used && steps_after_no_shells >= MAX_STEPS_AFTER_NO_SHELLS) {
            output_file << "Tie, both players have zero shells for " << MAX_STEPS_AFTER_NO_SHELLS << " steps" << std::endl;
        } else if (tanks1_alive == 0 && tanks2_alive == 0) {
            output_file << "Tie, both players have zero tanks" << std::endl;
        }

        LOG_DEBUG("Closing output file");
        output_file.close();
    }
}

// Render the current game state for debugging
std::string GameManager::renderGameState() const {
    if (!state) return "Error: Game state is null.\n";

    // Create a 2D char vector representing the board
    std::vector<std::vector<char>> board(state->board_height, std::vector<char>(state->board_width, '.')); // Use '.' for empty

    // Fill in objects, handling potential overlaps (last object drawn wins appearance)
    for (const auto& obj : state->objects) {
        if (obj && !obj->is_destroyed && state->isValidPosition(obj->position)) {
            // Ensure coordinates are within bounds before accessing vector
            if (obj->position.y >= 0 &&
                obj->position.y < state->board_height &&
                obj->position.x >= 0 &&
                obj->position.x < state->board_width) {
                board[obj->position.y][obj->position.x] = obj->render();
            } else {
                LOG_WARNING("Object " + std::string(1, obj->symbol) + " found at invalid position " + obj->position.toString() + " during render.");
            }
        }
    }

    // Convert board to string with borders
    std::string result = "+";
    for(int i = 0; i < state->board_width; ++i) result += '-';
    result += "+\n";

    for (const auto& row : board) {
        result += '|';
        for (char c : row) {
            result += c;
        }
        result += "|\n";
    }

    result += "+";
    for(int i = 0; i < state->board_width; ++i) result += '-';
    result += "+\n";

    return result;
}

// Get string representation of game result
std::string GameManager::getGameResultString() const {
    if (winner == "Tie") {
        return "Tie";
    } else if (winner == "Player 1" || winner == "Player 2") {
        return winner + " wins";
    } else {
        return "Undetermined"; // Should not happen if game over logic is correct
    }
}


void GameManager::checkInputWarnings(const std::string& file_path, const std::vector<std::string>& warnings) {
    if (!warnings.empty()) {
        LOG_INFO("Writing " + std::to_string(warnings.size()) + " input warnings to input_errors.txt");
        std::ofstream warning_file("input_errors.txt");
        if (warning_file.is_open()) {
            warning_file << "Warnings found during parsing of input file: " << file_path << std::endl;
            for (const auto& warning : warnings) {
                warning_file << "- " << warning << std::endl;
            }
            warning_file.close();
            LOG_INFO("Successfully wrote input warnings to input_errors.txt");
        } else {
            LOG_ERROR("Failed to open input_errors.txt for writing warnings");
        }
    } else {
        LOG_INFO("No input warnings found during parsing.");
    }
}

