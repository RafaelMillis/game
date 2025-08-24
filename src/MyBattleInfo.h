#ifndef TANKSBATTLEGAME_MYBATTLEINFO_H
#define TANKSBATTLEGAME_MYBATTLEINFO_H

#include <memory>
#include <vector>
#include <cstddef>

#include "common/BattleInfo.h"
#include "common/SatelliteView.h"

using std::shared_ptr;
using std::vector;
using std::size_t;

class MyBattleInfo : public BattleInfo {
private:
    shared_ptr<SatelliteView> satelliteView;
    size_t board_width;
    size_t board_height;
    int player_id;

public:
    MyBattleInfo(shared_ptr<SatelliteView> satelliteView, size_t board_width, size_t board_height, int player_id)
        : satelliteView(satelliteView)
        , board_width(board_width)
        , board_height(board_height)
        , player_id(player_id) {}

    ~MyBattleInfo() override = default;

    char getObjectAt(size_t x, size_t y) const {
        return satelliteView->getObjectAt(x, y);
    }

    size_t getBoardWidth() const { return board_width; }
    size_t getBoardHeight() const { return board_height; }

    int getPlayerId() const {
        return player_id;
    }
};

#endif //TANKSBATTLEGAME_MYBATTLEINFO_H 