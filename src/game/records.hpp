#pragma once

#include <cstdint>
#include <cstring>

namespace rogue {

constexpr int MAX_HALL_ENTRIES = 8;

enum AchievementBit : uint32_t {
    ACH_FIRST_BLOOD   = 1u << 0, // Slay 10 monsters in a run
    ACH_DEEP_DIVER    = 1u << 1, // Reach B5F
    ACH_ABYSSAL       = 1u << 2, // Reach B10F
    ACH_HOARDER       = 1u << 3, // Collect 500+ gold in a run
    ACH_SCHOLAR       = 1u << 4, // Identify 5+ item types in a run
    ACH_DRAGON_SLAYER = 1u << 5, // Slay a Dragon
    ACH_AMULET_BEARER = 1u << 6, // Pick up the Amulet of Yendor
    ACH_TRUE_ROGUE    = 1u << 7, // Escape with the Amulet of Yendor
    ACH_VETERAN       = 1u << 8, // Reach Character Level 7
    ACH_SURVIVOR      = 1u << 9, // Survive 1000+ turns in a run
};

struct AchievementInfo {
    AchievementBit bit;
    const char* name;
    const char* desc;
};

constexpr int NUM_ACHIEVEMENTS = 10;
const AchievementInfo* getAchievementList();

struct HallEntry {
    int32_t score = 0;
    int16_t floor = 0;
    int16_t level = 0;
    int32_t gold = 0;
    int32_t runNum = 0;
    bool escaped = false;
    char cause[28] = {0};
};

struct RecordsData {
    uint32_t magic = 0x524F4731; // "ROG1"
    uint16_t version = 1;
    int32_t bestScore = 0;
    int16_t bestFloor = 0;
    int16_t bestLevel = 0;
    int32_t bestGold = 0;
    int32_t totalRuns = 0;
    int32_t totalWins = 0;
    int32_t totalKills = 0;
    uint32_t achievements = 0;
    HallEntry hall[MAX_HALL_ENTRIES];
};

struct RunResult {
    int32_t score = 0;
    int16_t floor = 1;
    int16_t level = 1;
    int32_t gold = 0;
    int16_t kills = 0;
    int32_t turns = 0;
    bool escaped = false;
    bool hasAmulet = false;
    char cause[28] = {0};

    // Computed when recorded
    bool newBestScore = false;
    bool newBestFloor = false;
    bool newBestGold = false;
    bool newBestLevel = false;
    int hallRank = -1; // 0-based rank in Hall of Fame, or -1 if not top 8
    uint32_t newlyUnlockedAch = 0;
};

void initRecords(RecordsData& rec);
bool isValidRecords(const RecordsData& rec);
void recordRun(RecordsData& rec, RunResult& res, uint32_t runAchMask);
int countAchievements(uint32_t mask);

} // namespace rogue
