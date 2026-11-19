#include "dungeon.hpp"
#include <algorithm>
#include <array>
#include <vector>

namespace rogue {

void DungeonMap::clear() {
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            tiles[y][x] = Tile::Void;
            visible[y][x] = false;
            explored[y][x] = false;
        }
    }
    for (int i = 0; i < MAX_ROOMS; ++i) {
        rooms[i] = Room{};
        gold[i] = GoldPile{};
    }
    for (int i = 0; i < 12; ++i) {
        traps[i] = TrapCell{};
    }
    trapCount = 0;
    stairsDownX = stairsDownY = 0;
    stairsUpX = stairsUpY = 0;
    startX = startY = 0;
}

bool DungeonMap::isWalkable(int x, int y) const {
    if (!inBounds(x, y)) return false;
    Tile t = tiles[y][x];
    return t == Tile::Floor || t == Tile::Corridor || t == Tile::Door ||
           t == Tile::StairsDown || t == Tile::StairsUp || t == Tile::Trap;
}

bool DungeonMap::isOpaque(int x, int y) const {
    if (!inBounds(x, y)) return true;
    Tile t = tiles[y][x];
    return t == Tile::Void || t == Tile::WallH || t == Tile::WallV || t == Tile::SecretDoor;
}

int DungeonMap::roomAt(int x, int y) const {
    for (int i = 0; i < MAX_ROOMS; ++i) {
        const Room& r = rooms[i];
        if (!r.exists) continue;
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) {
            return i;
        }
    }
    return -1;
}

static void carveCorridorTile(DungeonMap& map, int x, int y, bool allowSecret, std::mt19937& rng) {
    if (!map.inBounds(x, y)) return;
    Tile t = map.tiles[y][x];
    if (t == Tile::Void) {
        map.tiles[y][x] = Tile::Corridor;
    } else if (t == Tile::WallH || t == Tile::WallV) {
        if (allowSecret && std::uniform_int_distribution<int>(0, 9)(rng) == 0) {
            map.tiles[y][x] = Tile::SecretDoor;
        } else {
            map.tiles[y][x] = Tile::Door;
        }
    }
}

static void connectPoints(DungeonMap& map, int x1, int y1, int x2, int y2, bool allowSecret, std::mt19937& rng) {
    int x = x1;
    int y = y1;
    bool horizFirst = std::uniform_int_distribution<int>(0, 1)(rng) == 0;
    int midX = (x1 + x2) / 2;
    int midY = (y1 + y2) / 2;

    if (horizFirst) {
        while (x != midX) {
            x += (midX > x) ? 1 : -1;
            carveCorridorTile(map, x, y, allowSecret, rng);
        }
        while (y != y2) {
            y += (y2 > y) ? 1 : -1;
            carveCorridorTile(map, x, y, allowSecret, rng);
        }
        while (x != x2) {
            x += (x2 > x) ? 1 : -1;
            carveCorridorTile(map, x, y, allowSecret, rng);
        }
    } else {
        while (y != midY) {
            y += (midY > y) ? 1 : -1;
            carveCorridorTile(map, x, y, allowSecret, rng);
        }
        while (x != x2) {
            x += (x2 > x) ? 1 : -1;
            carveCorridorTile(map, x, y, allowSecret, rng);
        }
        while (y != y2) {
            y += (y2 > y) ? 1 : -1;
            carveCorridorTile(map, x, y, allowSecret, rng);
        }
    }
}

static void randomFloorInRoom(const Room& r, int& ox, int& oy, std::mt19937& rng) {
    ox = std::uniform_int_distribution<int>(r.x + 1, r.x + r.w - 2)(rng);
    oy = std::uniform_int_distribution<int>(r.y + 1, r.y + r.h - 2)(rng);
}

void generateDungeon(DungeonMap& map, int floor, bool ascending, std::mt19937& rng) {
    map.clear();

    constexpr int secW = MAP_W / 3; // 20
    constexpr int secH = MAP_H / 3; // 7

    // Choose 0..2 gone rooms (represented as 1x1 corridor hubs or skipped)
    std::array<bool, MAX_ROOMS> gone{};
    int numGone = std::uniform_int_distribution<int>(0, 2)(rng);
    for (int i = 0; i < numGone; ++i) {
        gone[std::uniform_int_distribution<int>(0, MAX_ROOMS - 1)(rng)] = true;
    }

    int centerX[MAX_ROOMS]{};
    int centerY[MAX_ROOMS]{};
    std::vector<int> activeRooms;

    for (int idx = 0; idx < MAX_ROOMS; ++idx) {
        int gx = idx % 3;
        int gy = idx / 3;
        int sx = gx * secW;
        int sy = gy * secH;

        if (gone[idx]) {
            centerX[idx] = sx + secW / 2;
            centerY[idx] = sy + secH / 2;
            map.tiles[centerY[idx]][centerX[idx]] = Tile::Corridor;
            continue;
        }

        int rw = std::uniform_int_distribution<int>(6, secW - 3)(rng);
        int rh = std::uniform_int_distribution<int>(4, secH - 1)(rng);
        int rx = sx + std::uniform_int_distribution<int>(1, secW - rw - 1)(rng);
        int ry = sy + std::uniform_int_distribution<int>(0, secH - rh)(rng);

        Room& r = map.rooms[idx];
        r.x = static_cast<int16_t>(rx);
        r.y = static_cast<int16_t>(ry);
        r.w = static_cast<int16_t>(rw);
        r.h = static_cast<int16_t>(rh);
        r.exists = true;
        // Dark rooms start appearing from floor 2+
        r.dark = (floor >= 2) && (std::uniform_int_distribution<int>(0, 9)(rng) < std::min(5, floor));

        for (int y = ry; y < ry + rh; ++y) {
            for (int x = rx; x < rx + rw; ++x) {
                if (y == ry || y == ry + rh - 1) {
                    map.tiles[y][x] = Tile::WallH;
                } else if (x == rx || x == rx + rw - 1) {
                    map.tiles[y][x] = Tile::WallV;
                } else {
                    map.tiles[y][x] = Tile::Floor;
                }
            }
        }

        centerX[idx] = rx + rw / 2;
        centerY[idx] = ry + rh / 2;
        activeRooms.push_back(idx);
    }

    // Build spanning tree over all 9 grid nodes (including gone hubs) to guarantee full connectivity
    std::array<bool, MAX_ROOMS> visited{};
    std::vector<int> treeNodes;
    int startNode = std::uniform_int_distribution<int>(0, MAX_ROOMS - 1)(rng);
    visited[startNode] = true;
    treeNodes.push_back(startNode);

    bool allowSecret = (floor >= 3);

    while (treeNodes.size() < MAX_ROOMS) {
        std::vector<std::pair<int, int>> candidates;
        for (int u : treeNodes) {
            int ux = u % 3, uy = u / 3;
            const int dx[4] = {1, -1, 0, 0};
            const int dy[4] = {0, 0, 1, -1};
            for (int d = 0; d < 4; ++d) {
                int nx = ux + dx[d], ny = uy + dy[d];
                if (nx >= 0 && nx < 3 && ny >= 0 && ny < 3) {
                    int v = ny * 3 + nx;
                    if (!visited[v]) {
                        candidates.push_back({u, v});
                    }
                }
            }
        }
        auto edge = candidates[std::uniform_int_distribution<int>(0, static_cast<int>(candidates.size()) - 1)(rng)];
        visited[edge.second] = true;
        treeNodes.push_back(edge.second);
        connectPoints(map, centerX[edge.first], centerY[edge.first],
                      centerX[edge.second], centerY[edge.second], false, rng);
    }

    // Add 1-3 extra loop corridors
    int extra = std::uniform_int_distribution<int>(1, 3)(rng);
    for (int i = 0; i < extra; ++i) {
        int u = std::uniform_int_distribution<int>(0, MAX_ROOMS - 1)(rng);
        int ux = u % 3, uy = u / 3;
        int v = -1;
        if (ux < 2 && std::uniform_int_distribution<int>(0, 1)(rng) == 0) v = u + 1;
        else if (uy < 2) v = u + 3;
        else if (ux > 0) v = u - 1;
        if (v >= 0) {
            connectPoints(map, centerX[u], centerY[u], centerX[v], centerY[v], allowSecret, rng);
        }
    }

    // Pick start room and stairs room (distinct if >= 2 rooms)
    int startRoom = activeRooms[std::uniform_int_distribution<int>(0, static_cast<int>(activeRooms.size()) - 1)(rng)];
    int stairsRoom = startRoom;
    if (activeRooms.size() >= 2) {
        do {
            stairsRoom = activeRooms[std::uniform_int_distribution<int>(0, static_cast<int>(activeRooms.size()) - 1)(rng)];
        } while (stairsRoom == startRoom);
    }

    randomFloorInRoom(map.rooms[startRoom], map.startX, map.startY, rng);

    int sx, sy;
    do {
        randomFloorInRoom(map.rooms[stairsRoom], sx, sy, rng);
    } while (sx == map.startX && sy == map.startY);

    if (ascending) {
        map.tiles[sy][sx] = Tile::StairsUp;
        map.stairsUpX = sx;
        map.stairsUpY = sy;
    } else {
        map.tiles[sy][sx] = Tile::StairsDown;
        map.stairsDownX = sx;
        map.stairsDownY = sy;
    }

    // Gold piles in some rooms
    for (int rIdx : activeRooms) {
        if (std::uniform_int_distribution<int>(0, 99)(rng) < 45) {
            int gx, gy;
            randomFloorInRoom(map.rooms[rIdx], gx, gy, rng);
            if (map.tiles[gy][gx] == Tile::Floor && !(gx == map.startX && gy == map.startY)) {
                map.gold[rIdx].exists = true;
                map.gold[rIdx].x = static_cast<int16_t>(gx);
                map.gold[rIdx].y = static_cast<int16_t>(gy);
                int maxG = 15 + floor * 12;
                map.gold[rIdx].amount = static_cast<int16_t>(std::uniform_int_distribution<int>(5, maxG)(rng));
            }
        }
    }

    // Traps on deeper floors
    int maxTraps = std::min(8, floor / 2 + 1);
    int numTraps = (floor >= 2) ? std::uniform_int_distribution<int>(0, maxTraps)(rng) : 0;
    for (int i = 0; i < numTraps && map.trapCount < 12; ++i) {
        int rIdx = activeRooms[std::uniform_int_distribution<int>(0, static_cast<int>(activeRooms.size()) - 1)(rng)];
        int tx, ty;
        randomFloorInRoom(map.rooms[rIdx], tx, ty, rng);
        if (map.tiles[ty][tx] == Tile::Floor && !(tx == map.startX && ty == map.startY)) {
            map.tiles[ty][tx] = Tile::Trap;
            TrapCell& tc = map.traps[map.trapCount++];
            tc.exists = true;
            tc.x = static_cast<int16_t>(tx);
            tc.y = static_cast<int16_t>(ty);
            tc.discovered = false;
            tc.kind = static_cast<TrapKind>(std::uniform_int_distribution<int>(0, 4)(rng));
        }
    }
}

} // namespace rogue
