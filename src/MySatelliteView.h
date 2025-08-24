#ifndef TANKSBATTLEGAME_MYSATELLITEVIEW_H
#define TANKSBATTLEGAME_MYSATELLITEVIEW_H

#include "common/SatelliteView.h"
#include <vector>
#include <cstddef>
#include <utility>

using std::vector;
using std::size_t;
using std::move;

class MySatelliteView : public SatelliteView {
private:
    vector<vector<char>> board_matrix;

public:
    explicit MySatelliteView(vector<vector<char>> board_matrix) 
        : board_matrix(std::move(board_matrix)) {}

    ~MySatelliteView() override = default;

    char getObjectAt(size_t x, size_t y) const override {
        if (board_matrix.empty() || 
            x >= board_matrix.size() ||
            board_matrix[0].empty() ||
            y >= board_matrix[0].size()) {
            return '&'; // Return & for out of bounds
        }
        return board_matrix[x][y];
    }
};

#endif //TANKSBATTLEGAME_MYSATELLITEVIEW_H 