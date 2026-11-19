#pragma once

#include "types.hpp"
#include <cstdint>
#include <random>

namespace rogue {

struct DungeonMap {
    Tile tiles[MAP_H][MAP_W];
    bool visible[MAP_H][MAP_W];
    bool explored[MAP_H][MAP_W];
    Room rooms[MAX_ROOMS];
    GoldPile gold[MAX_ROOMS];
    TrapCell traps[12];
    int trapCount = 0;

    int stairsDownX = 0;
    int stairsDownY = 0;
    int stairsUpX = 0;
    int stairsUpY = 0;
    int startX = 0;
    int startY = 0;

    void clear();
    bool inBounds(int x, int y) const {
        return x >= 0 && x < MAP_W && y >= 0 && y < MAP_H;
    }
    bool isWalkable(int x, int y) const;
    bool isOpaque(int x, int y) const;
    int roomAt(int x, int y) const;
};

void generateDungeon(DungeonMap& map, int floor, bool ascending, std::mt19937& rng);

} // namespace rogue
