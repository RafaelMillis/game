#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <vector>

// Log levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    NONE  // Used to disable logging
};

class Logger {
public:
    // Singleton pattern
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // Initialize the logger
    void init(LogLevel console_level = LogLevel::INFO, 
              LogLevel file_level = LogLevel::DEBUG,
              const std::string& log_file = "game_log.txt") {
        console_log_level = console_level;
        file_log_level = file_level;
        
        if (file_level != LogLevel::NONE) {
            log_file_stream.open(log_file, std::ios::out);
            if (!log_file_stream.is_open()) {
                std::cerr << "Failed to open log file: " << log_file << std::endl;
                file_log_level = LogLevel::NONE;
            }
        }
        
        is_initialized = true;
    }

    // Log a message with the specified level
    void log(LogLevel level, const std::string& message, 
             const std::string& file, int line, const std::string& function) {
        if (!is_initialized) {
            init();  // Initialize with default settings if not initialized
        }

        if (level < console_log_level && level < file_log_level) {
            return;  // Skip if below both console and file log levels
        }

        // Create the log message with timestamp and metadata
        std::string formatted_message = formatLogMessage(level, message, file, line, function);

        // Lock to ensure thread safety
        std::lock_guard<std::mutex> lock(log_mutex);

        // Log to console if level is sufficient
        if (level >= console_log_level) {
            std::ostream& output = (level >= LogLevel::WARNING) ? std::cerr : std::cout;
            output << formatted_message << std::endl;
        }

        // Log to file if level is sufficient and file is open
        if (level >= file_log_level && log_file_stream.is_open()) {
            log_file_stream << formatted_message << std::endl;
            log_file_stream.flush();  // Ensure it's written immediately
        }
    }

    // Get string representation of log level
    static std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR:   return "ERROR";
            case LogLevel::NONE:    return "DoNothing";
            default:                return "UNKNOWN";
        }
    }

    // Set console log level
    void setConsoleLogLevel(LogLevel level) {
        console_log_level = level;
    }

    // Set file log level
    void setFileLogLevel(LogLevel level) {
        file_log_level = level;
    }

    // Close the log file
    void closeLogFile() {
        if (log_file_stream.is_open()) {
            log_file_stream.close();
        }
    }

    // Destructor
    ~Logger() {
        closeLogFile();
    }

private:
    // Private constructor (singleton)
    Logger() : is_initialized(false), 
               console_log_level(LogLevel::INFO), 
               file_log_level(LogLevel::DEBUG) {}

    // Copy and move constructors and assignment operators are deleted
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // Format the log message with timestamp and metadata
    std::string formatLogMessage(LogLevel level, const std::string& message, 
                                 const std::string& file, int line, const std::string& function) {
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
        
        // Format as: [TIME] [LEVEL] [FILE:LINE][FUNCTION] MESSAGE
        std::stringstream formatted;
        formatted << "[" << ss.str() << "] "
                 << "[" << levelToString(level) << "] "
                 << "[" << file << ":" << line << "][" << function << "] "
                 << message;
        
        return formatted.str();
    }

    bool is_initialized;
    LogLevel console_log_level;
    LogLevel file_log_level;
    std::ofstream log_file_stream;
    std::mutex log_mutex;  // For thread safety
};

// Helper macros for easier logging
#define LOG_DEBUG(message) \
    Logger::getInstance().log(LogLevel::DEBUG, message, __FILE__, __LINE__, __FUNCTION__)

#define LOG_INFO(message) \
    Logger::getInstance().log(LogLevel::INFO, message, __FILE__, __LINE__, __FUNCTION__)

#define LOG_WARNING(message) \
    Logger::getInstance().log(LogLevel::WARNING, message, __FILE__, __LINE__, __FUNCTION__)

#define LOG_ERROR(message) \
    Logger::getInstance().log(LogLevel::ERROR, message, __FILE__, __LINE__, __FUNCTION__)

#endif // LOGGER_H
