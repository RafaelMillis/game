#include "MyTankAlgorithmFactory.h"
#include "RotatingAlgorithm.h"
#include "ChasingAlgorithm.h"
#include "InteractiveAlgorithm.h"
#include <fstream>
#include <memory>
#include <stdexcept>
#include <filesystem>
#include <cctype>

MyTankAlgorithmFactory::MyTankAlgorithmFactory() {
    LOG_INFO("MyTankAlgorithmFactory constructed");
    // Do not read algorithm types here; wait until executable_dir_path is set
}

void MyTankAlgorithmFactory::updateExecutableDirPath(const std::string& path) {
    executable_dir_path = path;
    // Now that the path is set, read the algorithm types
    auto [algo1_type, algo2_type] = readAlgorithmTypes();
    player1AlgoType = algo1_type;
    player2AlgoType = algo2_type;
}

std::pair<std::string, std::string> MyTankAlgorithmFactory::readAlgorithmTypes() const {
    std::filesystem::path config_path = std::filesystem::path(executable_dir_path) / "algorithm_types.txt";
    std::ifstream infile(config_path);
    std::string type1, type2;
    if (infile) {
        std::getline(infile, type1);
        std::getline(infile, type2);
    }
    LOG_INFO("Algorithm 1 type " + type1 + " Algorithm 2 type " + type2);
    return {type1, type2};
}

std::unique_ptr<TankAlgorithm> MyTankAlgorithmFactory::create(int player_index, int tank_index) const {
    LOG_INFO("Should make use of: `" + std::to_string(tank_index) + "` Tank index");
    std::string algoType = (player_index == 1) ? player1AlgoType : player2AlgoType;
    if (algoType == "rotating") {
        LOG_INFO("Player " + std::to_string(player_index) + " RotatingAlgorithm");
        return std::make_unique<RotatingAlgorithm>();
    } if (algoType == "interactive") {
        LOG_INFO("Player " + std::to_string(player_index) + " InteractiveAlgorithm");
        return std::make_unique<InteractiveAlgorithm>(player_index, tank_index);
    } else { // Default to chasing
        LOG_INFO("Player " + std::to_string(player_index) + " ChasingAlgorithm");
        return std::make_unique<ChasingAlgorithm>();
    }
}

void MyTankAlgorithmFactory::setAlgorithmTypes(const std::string& a1, const std::string& a2) {
    auto to_lower = [](std::string s){ for (auto &c : s) c = static_cast<char>(::tolower(static_cast<unsigned char>(c))); return s; };
    player1AlgoType = to_lower(a1);
    player2AlgoType = to_lower(a2);
    LOG_INFO("Algorithm types set via CLI: P1=" + player1AlgoType + ", P2=" + player2AlgoType);
}
