#ifndef MYTANKALGORITHMFACTORY_H
#define MYTANKALGORITHMFACTORY_H

#include "common/TankAlgorithmFactory.h"
#include <memory>
#include <string>

class MyTankAlgorithmFactory : public TankAlgorithmFactory {
public:
    MyTankAlgorithmFactory();
    std::unique_ptr<TankAlgorithm> create(int player_index, int tank_index) const override;
    void updateExecutableDirPath(const std::string& path);
public:
    void setAlgorithmTypes(const std::string& algo1, const std::string& algo2);
private:
    std::string player1AlgoType;
    std::string player2AlgoType;
    std::string executable_dir_path;
    std::pair<std::string, std::string> readAlgorithmTypes() const;
};

#endif // MYTANKALGORITHMFACTORY_H