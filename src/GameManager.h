// GameManager.h
#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <cstddef>         // For size_t
#include <memory>          // For std::unique_ptr, std::shared_ptr
#include <string>          // For std::string
#include <vector>          // For std::vector
#include <unordered_map>   // For std::unordered_map
#include <fstream>         // For std::ofstream
#include <unordered_set>   // For std::unordered_set
#include <utility>         // For std::pair
#include <exception>       // For std::exception
#include <stdexcept>      // For std::runtime_error
#include <filesystem>      // For std::filesystem
#include <iostream>        // For std::cout, std::endl

#include "GameState.h"
#include "common/TankAlgorithm.h"
#include "common/Player.h"
#include "common/TankAlgorithmFactory.h"
#include "common/PlayerFactory.h"
#include "Logger.h"
#include "common/ActionRequest.h"        // Include ActionRequest enum
#include "Position.h"      // Include Position struct/class
#include "ActionOutcome.h"

// Manages the overall game flow, state, and interactions
class GameManager {
public:
    /**
     * @brief Constructor initializes the game manager with a TankAlgorithmFactory (does NOT parse the input board file or instantiate algorithms).
     * @param tankAlgorithmFactory Reference to a TankAlgorithmFactory used to create tank algorithms.
     * @param playerFactory Reference to a PlayerFactory used to create players.
     * @param write_debug_file Whether to write debug information to a file.
     */
    GameManager(const TankAlgorithmFactory& tankAlgorithmFactory, const PlayerFactory& playerFactory, bool write_debug_file = false);

    /**
     * @brief Destructor. Ensures the output file stream is closed.
     */
    ~GameManager();

    /**
     * @brief Reads and initializes the game board from the input file. Must be called before run().
     *        Instantiates algorithms based on types read from algorithm_types.txt.
     * @param input_file_path Path to the text file describing the initial game board.
     * @throws std::runtime_error if the input file cannot be opened or contains critical errors.
     */
    void readBoard(const std::string& input_file_path);

    /**
     * @brief Starts and runs the main game loop until a game over condition is met.
     */
    void run();

private:
    // --- Nested Struct for Logged ActionRequest ---
    /**
     * @brief Structure to hold information about a logged action.
     */
    struct LoggedAction {
        int player_id = 0; // 0 indicates no action logged yet
        int tank_id = 0;   // The ID of the tank that performed the action
        ActionRequest action = ActionRequest::DoNothing;
        bool is_bad = false;
        bool was_tank_destroyed = false; // Whether the tank was destroyed at the time of the action
        bool killed_this_step = false;   // Whether the tank was killed in this specific step
        std::shared_ptr<Tank> tank = nullptr; // The tank that performed the action
    };

    /**
     * @brief Structure to hold intermediate data for a tank during a step.
     */
    struct TankStepData {
        std::shared_ptr<Tank> tank = nullptr;
        ActionRequest action = ActionRequest::DoNothing;
        ActionOutcome outcome = ActionOutcome::NONE;
        Position intended_position; // Stores the calculated position for the next sub-step/end of step
        LoggedAction logged_action; // Stores action details for logging at the end
        bool blocked_this_sub_step = false; // Flag if movement was blocked in the current sub-step
    };

    // --- Member Variables ---
    std::shared_ptr<GameState> state;             // Holds the current state of all game objects
    std::unordered_map<int, std::unique_ptr<TankAlgorithm>> algorithms; // Map of tank_id to algorithm
    std::unique_ptr<Player> player1;              // Player 1 instance
    std::unique_ptr<Player> player2;              // Player 2 instance

    // Game configuration/rules
    const int MAX_STEPS_AFTER_NO_SHELLS = 40; // Max steps allowed after shells run out
    int max_game_steps = 1000; // Max steps for the game to run (configurable from map)
    int num_shells = 16;       // Number of shells per tank (configurable from map)
    int num_rows = 0;          // Number of rows in map (from map file)
    int num_cols = 0;          // Number of columns in map (from map file)
    std::string map_name;      // Map name/description (from map file)

    // Game progress state
    int steps_count = 0;                // Current step number
    bool game_over = false;             // Flag indicating if the game has ended
    std::string winner;                 // Stores the winner ("Player 1", "Player 2", "Tie")
    std::string game_over_reason;       // Explanation for why the game ended

    // Tracking for end-game condition
    bool all_shells_used = false;       // Flag if both tanks have used all shells
    int steps_after_no_shells = 0;

    // Output handling
    std::ofstream output_file;          // Stream to write game progress and results
    std::ofstream debug_file;           // Stream to write debug information (only used if write_debug_file is true)
    std::vector<LoggedAction> action_log;

    const TankAlgorithmFactory& tankAlgorithmFactory;
    const PlayerFactory& playerFactory;
    bool write_debug_file;              // Flag to control debug file writing

    std::unordered_map<int, std::shared_ptr<Tank>> tanks; // Map of tank_id to Tank

    // --- Private Helper Methods ---

    /**
     * @brief Parses the input file to initialize the game board and objects.
     * @param file_path Path to the input file.
     * @throws std::runtime_error for critical parsing errors (missing dimensions, missing tanks).
     */
    void parseInputFile(const std::string& file_path);

    /**
     * @brief Processes a single step (turn) of the game.
     * Gets actions, moves objects, handles collisions, checks game over.
     */
    void processStep();

    /**
     * @brief Checks for game over conditions (tank destroyed, max steps reached).
     * @return True if the game is over, false otherwise.
     */
    bool checkGameOver();

    /**
     * @brief Writes the final game results (winner, steps, reason) to the output file.
     */
    void writeGameResults();

    /**
     * @brief Renders the current game state grid to a string (for output/debugging).
     * @return A string representation of the game board.
     */
    std::string renderGameState() const;

    /**
     * @brief Logs the action performed by a player to the internal action log and output file.
     * @param logged_action The action details to log.
     */
    void logAction(const LoggedAction& logged_action);

    /**
     * @brief Writes any non-critical warnings found during input parsing to 'input_errors.txt'.
     * @param file_path The path of the input file parsed.
     * @param warnings A vector of warning messages generated during parsing.
     */
    static void checkInputWarnings(const std::string& file_path, const std::vector<std::string>& warnings);

    /**
     * @brief Removes objects marked as destroyed from the game state's object list.
     */
    void cleanupDestroyedObjects();

    /**
     * @brief Gets a string describing the game result (e.g., "Player 1 wins", "Tie").
     * @return String representation of the game outcome.
     */
    std::string getGameResultString() const;

    void writeGameStartOutput();

    void startGameLoop();

    void executeImmediateActions(std::unordered_map<int, TankStepData>& tank_data);

    void processTankTransitions(std::unordered_map<int, TankStepData>& tank_data);

    void getPlayerActions(std::unordered_map<int, TankStepData>& tank_data);

    void prepareStep(std::unordered_map<int, TankStepData>& tank_data);

    void finalizeStep();

    void checkShellDepletionStatus();

    void runMovementAndCollisionSubSteps(std::unordered_map<int, TankStepData>& tank_data);

    void calculateIntendedTankPositionsSubStep(std::unordered_map<int, TankStepData>& tank_data, int max_speed);

    std::vector<std::pair<std::shared_ptr<Shell>, Position>> calculateShellIntendedPositionsSubStep(int max_speed);

    void updateObjectPositionsSubStep(std::unordered_map<int, TankStepData>& tank_data,
                                      const std::vector<std::pair<std::shared_ptr<Shell>, Position>>& shells_intended_positions);

    void resolveCollisionsSubStep(std::unordered_map<int, TankStepData>& tank_data,
                                  std::vector<std::pair<std::shared_ptr<Shell>, Position>>& shells_intended_positions);

    void resolveShellWallCollisions(std::vector<std::pair<std::shared_ptr<Shell>, Position>>& shells_intended_positions,
                                    const std::unordered_set<Position>& wall_positions);

    void resolveTankWallCollisions(std::unordered_map<int, TankStepData>& tank_data,
                                   const std::unordered_set<Position>& wall_positions);

    void resolveShellShellCollisions(std::vector<std::pair<std::shared_ptr<Shell>, Position>>& shells_intended_positions);

    void resolveShellTankCollisions(std::unordered_map<int, TankStepData>& tank_data,
                                    std::vector<std::pair<std::shared_ptr<Shell>, Position>>& shells_intended_positions);

    void resolveTankMineCollisions(std::unordered_map<int, TankStepData>& tank_data,
                                   const std::unordered_set<Position>& mine_positions);

    void resolveTankTankCollisions(std::unordered_map<int, TankStepData>& tank_data);
};

#endif // GAMEMANAGER_H
