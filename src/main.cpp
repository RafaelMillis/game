// main.cpp
#include "GameManager.h" // Assuming GameManager.h exists and is needed
#include "Logger.h"
#include <iostream>
#include <string>
#include <cstring> // For strcmp, strncmp
#include <filesystem> // For std::filesystem::exists
#include <cctype> // For std::tolower
#include "MyTankAlgorithmFactory.h"
#include "MyPlayerFactory.h"

using namespace std;

void printUsage() {
    cout << "Usage: tanks_game <game_board_input_file> [options]" << endl;
    cout << "Options:" << endl;
    cout << "  --log-level <level>      Set log level to both console and log file (DEBUG, INFO, WARNING, ERROR, DoNothing)" << endl;
    cout << "  --algorithm1=<type>      Set algorithm for Player 1 (rotating, interactive, chasing, simple)" << endl;
    cout << "  --algorithm2=<type>      Set algorithm for Player 2 (rotating, interactive, chasing, simple)" << endl;
    cout << "  --write_debug_file       Enable writing detailed debug information to a file" << endl;
    cout << "  --help, -h               Show this help message" << endl;
}

LogLevel parseLogLevel(const std::string& level_str) {
    std::string level = level_str;
    // Convert to uppercase
    std::transform(level.begin(), level.end(), level.begin(),
                   [](unsigned char c){ return std::toupper(c); }); // Use lambda for toupper

    if (level == "DEBUG") return LogLevel::DEBUG;
    if (level == "INFO") return LogLevel::INFO;
    if (level == "WARNING") return LogLevel::WARNING;
    if (level == "ERROR") return LogLevel::ERROR;
    if (level == "DoNothing") return LogLevel::NONE;

    // Return a default
    return LogLevel::NONE; // Default to DoNothing on parse error
}

int main(int argc, char* argv[]) {
    // --- Early argument check (before logger is initialized) ---
    if (argc < 2 || (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0))) {
        printUsage();
        return (argc < 2); // Return 1 if missing file, 0 if --help
    }

    std::string input_file_path = argv[1];

    // Verify input file exists before proceeding
    if (!std::filesystem::exists(input_file_path)) {
        std::cerr << "Error: Input file does not exist: " << input_file_path << std::endl;
        return 1;
    }

    // --- Default Settings ---
    LogLevel cmd_console_log_level = LogLevel::NONE; // Use DoNothing to indicate "not set via command line"
    bool write_debug_file = false; // Default to not writing debug file
    std::string log_file_name = "logger_output.txt"; // Default log file name

    // --- Parse optional arguments (starting from index 2) ---
    std::string algo1_type = "chasing"; // default
    std::string algo2_type = "chasing"; // default
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--log-level") == 0 && i + 1 < argc) {
            cmd_console_log_level = parseLogLevel(argv[++i]); // Increment i after use
        }
        else if (strcmp(argv[i], "--write_debug_file") == 0) {
            write_debug_file = true;
        }
        else if (std::strncmp(argv[i], "--algorithm1=", 13) == 0) {
            algo1_type = std::string(argv[i] + 13);
        }
        else if (std::strncmp(argv[i], "--algorithm2=", 13) == 0) {
            algo2_type = std::string(argv[i] + 13);
        }
        else {
            // Handle unknown arguments (logging might not be ready yet)
            std::cerr << "Warning: Unknown command line argument ignored: " << argv[i] << std::endl;
        }
    }

    // --- Determine Final Logger Settings ---
    // Use command-line value if set (not DoNothing), otherwise use hardcoded default for console
    LogLevel final_console_log_level = (cmd_console_log_level != LogLevel::NONE) ? cmd_console_log_level : LogLevel::NONE;

    // --- Initialize Logger ---
    // Use the determined levels and the default log file name
    Logger::getInstance().init(final_console_log_level, final_console_log_level, log_file_name);

    // --- Now Logger is available ---
    LOG_INFO("--------------------------------------------------");
    LOG_INFO("Starting Tanks Game");
    LOG_INFO("Input file: " + input_file_path);
    LOG_INFO("Console Log Level: " + Logger::levelToString(final_console_log_level));
    LOG_INFO("File Log Level: " + Logger::levelToString(final_console_log_level)); // Log the default file level
    LOG_INFO("Log File: " + log_file_name);
    LOG_INFO("Debug File Writing: " + std::string(write_debug_file ? "enabled" : "disabled"));

    try {
        // --- Create the game manager and run the game ---
        LOG_INFO("Creating GameManager...");
        MyTankAlgorithmFactory factory;
        MyPlayerFactory playerFactory;
        // Determine executable directory
        std::filesystem::path exe_path = std::filesystem::absolute(argv[0]);
        std::string exe_dir = exe_path.parent_path().lexically_normal().string();
        factory.updateExecutableDirPath(exe_dir);
        factory.setAlgorithmTypes(algo1_type, algo2_type);
        GameManager gameManager(factory, playerFactory, write_debug_file);

        LOG_INFO("Running game simulation...");
        gameManager.readBoard(input_file_path);
        gameManager.run(); // Execute the main game loop

        LOG_INFO("Game completed successfully.");

    } catch (const std::exception& e) {
        // Log the error before exiting
        LOG_ERROR("Critical Error during game execution: " + std::string(e.what()));
        std::cerr << "Error: " << e.what() << std::endl;
        Logger::getInstance().closeLogFile(); // Ensure logs are flushed
        return 1; // Indicate failure
    } catch (...) {
        // Log an unknown error before exiting
        LOG_ERROR("An unknown error occurred during game execution.");
        std::cerr << "An unknown error occurred." << std::endl;
        Logger::getInstance().closeLogFile(); // Ensure logs are flushed
        return 1; // Indicate failure
    }

    // --- Clean Shutdown ---
    LOG_INFO("Game execution finished. Shutting down.");
    Logger::getInstance().closeLogFile(); // Close the log file stream

    return 0; // Indicate success
}
