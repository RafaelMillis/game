#ifndef POSITION_H
#define POSITION_H

#include <string>
#include <iostream>
#include <sstream>

class Position {
public:
    int x;
    int y;

    Position(int x_pos = 0, int y_pos = 0) : x(x_pos), y(y_pos) {}

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Position& other) const {
        return !(*this == other);
    }

    Position operator+(const Position& other) const {
        return Position(x + other.x, y + other.y);
    }

    std::string toString() const { std::ostringstream oss; oss<<"("<<x<<","<<y<<")"; return oss.str(); }

    friend std::ostream& operator<<(std::ostream& os, const Position& pos) {
        os << pos.toString();
        return os;
    }
};

// For use in unordered maps/sets
namespace std {
    template<>
    struct hash<Position> {
        int operator()(const Position& pos) const {
            return hash<int>()(pos.x) ^ (hash<int>()(pos.y) << 1);
        }
    };
}

#endif // POSITION_H