#include "ChasingAlgorithm.h"
#include "Logger.h"
#include "GameManager.h"
#include "common/SatelliteView.h"
#include "Direction.h"
#include <cstddef>
#include <vector>
#include <memory>
#include <optional>
#include <cmath>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <limits>
#include <string>

using std::vector;
using std::optional;
using std::unordered_map;
using std::unordered_set;
using std::queue;
using std::string;
using std::stringstream;

std::optional<Direction> calculateDirection(const Position& from, const Position& to, [[maybe_unused]]int board_width, [[maybe_unused]]int board_height) {
    int dx = to.x - from.x;
    int dy = to.y - from.y;

    if (dx == 0 && dy == 0) return std::nullopt; // Same position

    // Determine primary direction based on largest magnitude
    if (std::abs(dx) > std::abs(dy)) { // More horizontal movement
        if (dx > 0) { // Moving Right
            if (dy > dx / 2.0) return Direction::DOWN_RIGHT;
            if (dy < -dx / 2.0) return Direction::UP_RIGHT;
            return Direction::RIGHT;
        } else { // Moving Left
            if (dy > -dx / 2.0) return Direction::DOWN_LEFT;
            if (dy < dx / 2.0) return Direction::UP_LEFT;
            return Direction::LEFT;
        }
    } else { // More vertical movement (or equal)
        if (dy > 0) { // Moving Down
            if (dx > dy / 2.0) return Direction::DOWN_RIGHT;
            if (dx < -dy / 2.0) return Direction::DOWN_LEFT;
            return Direction::DOWN;
        } else { // Moving Up
            if (dx > -dy / 2.0) return Direction::UP_RIGHT;
            if (dx < dy / 2.0) return Direction::UP_LEFT;
            return Direction::UP;
        }
    }
}

ActionRequest getShortestRotation(Direction current, Direction target) {
    int current_idx = static_cast<int>(current);
    int target_idx = static_cast<int>(target);
    int diff = target_idx - current_idx;

    // Normalize difference to be between -4 and 4
    if (diff > 4) diff -= 8;
    if (diff <= -4) diff += 8;

    if (diff == 0) return ActionRequest::DoNothing; // Already facing target

    // Prefer 45-degree rotations if possible
    if (diff == 1 || diff == -7) return ActionRequest::RotateRight45;
    if (diff == -1 || diff == 7) return ActionRequest::RotateLeft45;
    if (diff == 2 || diff == -6) return ActionRequest::RotateRight90;
    if (diff == -2 || diff == 6) return ActionRequest::RotateLeft90;
    if (diff == 3 || diff == -5) return ActionRequest::RotateRight90; // Choose 90 for 135
    if (diff == -3 || diff == 5) return ActionRequest::RotateLeft90; // Choose 90 for -135
    if (std::abs(diff) == 4) return ActionRequest::RotateRight90; // 180 degree turn, pick one (e.g., right)

    return ActionRequest::DoNothing; // Should not happen with normalization
}

bool isBlocked(bool isPosOccupiedByWall, bool isPosOccupiedByMine) {
    return isPosOccupiedByWall || isPosOccupiedByMine;
}

std::vector<Position> reconstructPath(
        const Position& start,
        const Position& goal,
        const std::unordered_map<Position, Position>& came_from)
{
    std::vector<Position> path;
    Position current = goal;
    // Check if goal is reachable (present in came_from or is the start itself)
    if (came_from.find(goal) == came_from.end() && start != goal) {
        return path; // Return empty path if goal not reached
    }

    while (current != start) {
        path.push_back(current);
        // Check if current node exists in came_from map before accessing
        auto it = came_from.find(current);
        if (it == came_from.end()) {
            // Should not happen if goal was reachable, but added as safety
            LOG_ERROR("Path reconstruction failed: Node not found in came_from map.");
            return {}; // Return empty path on error
        }
        current = it->second;
    }
    // path.push_back(start); // Optional: include start in the path
    std::reverse(path.begin(), path.end()); // Reverse to get path from start to goal
    return path;
}

bool isLineOfSightClear(const Position& start, const Position& end, int board_width, int board_height,
                       const std::vector<std::vector<char>>& board_matrix) {
    LOG_DEBUG("LoS Check: From " + start.toString() + " to " + end.toString());

    // Calculate direct path differences - no wrapping
    int dx = end.x - start.x;
    int dy = end.y - start.y;

    if (dx == 0 && dy == 0) return true;

    int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) return true;

    double x_step = static_cast<double>(dx) / steps;
    double y_step = static_cast<double>(dy) / steps;

    for (int i = 1; i < steps; ++i) {
        double exact_x = start.x + (x_step * i);
        double exact_y = start.y + (y_step * i);

        int check_x = static_cast<int>(std::round(exact_x));
        int check_y = static_cast<int>(std::round(exact_y));

        // Skip if outside board boundaries
        if (check_x < 0 || check_x >= board_width || check_y < 0 || check_y >= board_height) {
            continue;
        }

        Position check_pos(check_x, check_y);
        if (check_pos == start || check_pos == end) continue;

        // Check for walls
        char obj = board_matrix[check_x][check_y];
        if (obj == '#') {
            LOG_DEBUG("LoS Check: Blocked by wall at " + check_pos.toString());
            return false;
        }
    }

    LOG_DEBUG("LoS Check: Path clear.");
    return true;
}

// Helper function to check if two positions are exactly aligned (same x or y coordinate)
bool isExactlyAligned(const Position& from, const Position& to) {
    return from.x == to.x || from.y == to.y;
}