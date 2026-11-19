#include "../src/game/dungeon.hpp"
#include "../src/game/records.hpp"
#include "../src/game/rogue.hpp"

#include <cassert>
#include <cstdio>
#include <queue>
#include <random>

using namespace rogue;

static void testDungeonConnectivity() {
    for (int seed = 1; seed <= 50; ++seed) {
        std::mt19937 rng(seed);
        DungeonMap map;
        int floor = (seed % AMULET_FLOOR) + 1;
        bool asc = (seed % 5 == 0);
        generateDungeon(map, floor, asc, rng);

        assert(map.isWalkable(map.startX, map.startY));
        int sx = asc ? map.stairsUpX : map.stairsDownX;
        int sy = asc ? map.stairsUpY : map.stairsDownY;
        assert(map.isWalkable(sx, sy));

        bool visited[MAP_H][MAP_W]{};
        std::queue<std::pair<int, int>> q;
        q.push({map.startX, map.startY});
        visited[map.startY][map.startX] = true;

        const int dx[4] = {1, -1, 0, 0};
        const int dy[4] = {0, 0, 1, -1};
        while (!q.empty()) {
            auto [x, y] = q.front();
            q.pop();
            for (int d = 0; d < 4; ++d) {
                int nx = x + dx[d], ny = y + dy[d];
                if (map.inBounds(nx, ny) && !visited[ny][nx] && map.isWalkable(nx, ny)) {
                    visited[ny][nx] = true;
                    q.push({nx, ny});
                }
            }
        }

        assert(visited[sy][sx]);
        for (int i = 0; i < MAX_ROOMS; ++i) {
            if (map.rooms[i].exists) {
                int rx = map.rooms[i].x + 1;
                int ry = map.rooms[i].y + 1;
                assert(visited[ry][rx]);
            }
        }
    }
    std::printf("[PASS] testDungeonConnectivity (50 seeds)\n");
}

static void testGameplayAndInventory() {
    RecordsData rec;
    initRecords(rec);
    rec.bestFloor = 3;

    RogueGame game;
    game.startNewGame(42, rec);
    assert(game.outcome() == GameOutcome::Playing);
    assert(game.player().floor == 1);
    assert(game.inventory().size() == 4); // Mace, Ring Mail, Food, Healing Potion

    // Test rest consumes turns and hunger
    int h0 = game.player().hunger;
    game.step(Action::Rest);
    assert(game.player().turns == 1);
    assert(game.player().hunger == h0 - 1);

    // Eat food at index 2
    bool ate = game.useInventoryItem(2);
    assert(ate);
    assert(game.player().hunger >= h0);
    assert(game.inventory().size() == 3);
    // Equipped indices must still be valid
    assert(game.player().wieldedIdx == 0);
    assert(game.player().armorIdx == 1);

    // Drop healing potion at index 2
    bool dropped = game.dropInventoryItem(2);
    assert(dropped);
    assert(game.inventory().size() == 2);
    assert(!game.groundItems().empty());

    std::printf("[PASS] testGameplayAndInventory\n");
}

static void testRecordsAndHallOfFame() {
    RecordsData rec;
    initRecords(rec);
    assert(isValidRecords(rec));

    RunResult r1;
    r1.score = 450;
    r1.floor = 4;
    r1.level = 3;
    r1.gold = 120;
    r1.kills = 8;
    std::snprintf(r1.cause, sizeof(r1.cause), "Killed by Orc");
    recordRun(rec, r1, ACH_FIRST_BLOOD);

    assert(r1.newBestScore);
    assert(r1.newBestFloor);
    assert(r1.hallRank == 0);
    assert(rec.totalRuns == 1);
    assert(rec.bestFloor == 4);
    assert((r1.newlyUnlockedAch & ACH_FIRST_BLOOD) != 0);

    RunResult r2;
    r2.score = 1200;
    r2.floor = 9;
    r2.level = 6;
    r2.gold = 340;
    r2.kills = 25;
    std::snprintf(r2.cause, sizeof(r2.cause), "Killed by Troll");
    recordRun(rec, r2, ACH_FIRST_BLOOD | ACH_DEEP_DIVER);

    assert(r2.newBestScore);
    assert(r2.newBestFloor);
    assert(r2.hallRank == 0);
    assert(rec.hall[0].score == 1200);
    assert(rec.hall[1].score == 450);
    assert((r2.newlyUnlockedAch & ACH_FIRST_BLOOD) == 0);
    assert((r2.newlyUnlockedAch & ACH_DEEP_DIVER) != 0);

    std::printf("[PASS] testRecordsAndHallOfFame\n");
}

int main() {
    testDungeonConnectivity();
    testGameplayAndInventory();
    testRecordsAndHallOfFame();
    std::printf("All host tests passed!\n");
    return 0;
}
