#include "records.hpp"
#include <algorithm>
#include <cstdio>

namespace rogue {

static const AchievementInfo kAchievements[NUM_ACHIEVEMENTS] = {
    {ACH_FIRST_BLOOD,   "First Blood",   "Slay 10 monsters in a run"},
    {ACH_DEEP_DIVER,    "Deep Diver",    "Reach dungeon floor B5F"},
    {ACH_ABYSSAL,       "Abyssal",       "Reach dungeon floor B10F"},
    {ACH_HOARDER,       "Hoarder",       "Collect 500+ gold in a run"},
    {ACH_SCHOLAR,       "Scholar",       "Identify 5+ items in a run"},
    {ACH_DRAGON_SLAYER, "Dragon Slayer", "Slay a mighty Dragon"},
    {ACH_AMULET_BEARER, "Amulet Bearer", "Claim the Amulet of Yendor"},
    {ACH_TRUE_ROGUE,    "True Rogue",    "Escape with the Amulet!"},
    {ACH_VETERAN,       "Veteran",       "Reach character level 7"},
    {ACH_SURVIVOR,      "Survivor",      "Survive 1000+ turns in a run"},
};

const AchievementInfo* getAchievementList() {
    return kAchievements;
}

void initRecords(RecordsData& rec) {
    rec = RecordsData{};
    rec.magic = 0x524F4731;
    rec.version = 1;
    for (int i = 0; i < MAX_HALL_ENTRIES; ++i) {
        rec.hall[i] = HallEntry{};
    }
}

bool isValidRecords(const RecordsData& rec) {
    return rec.magic == 0x524F4731 && rec.version == 1 && rec.totalRuns >= 0;
}

void recordRun(RecordsData& rec, RunResult& res, uint32_t runAchMask) {
    if (!isValidRecords(rec)) {
        initRecords(rec);
    }

    rec.totalRuns += 1;
    if (res.escaped) {
        rec.totalWins += 1;
    }
    rec.totalKills += res.kills;

    if (res.score > rec.bestScore) {
        rec.bestScore = res.score;
        res.newBestScore = true;
    }
    if (res.floor > rec.bestFloor) {
        rec.bestFloor = res.floor;
        res.newBestFloor = true;
    }
    if (res.gold > rec.bestGold) {
        rec.bestGold = res.gold;
        res.newBestGold = true;
    }
    if (res.level > rec.bestLevel) {
        rec.bestLevel = res.level;
        res.newBestLevel = true;
    }

    res.newlyUnlockedAch = runAchMask & (~rec.achievements);
    rec.achievements |= runAchMask;

    // Insert into Hall of Fame if score > 0 or better than lowest entry
    HallEntry entry;
    entry.score = res.score;
    entry.floor = res.floor;
    entry.level = res.level;
    entry.gold = res.gold;
    entry.runNum = rec.totalRuns;
    entry.escaped = res.escaped;
    std::snprintf(entry.cause, sizeof(entry.cause), "%s", res.cause);

    int insertPos = -1;
    for (int i = 0; i < MAX_HALL_ENTRIES; ++i) {
        if (rec.hall[i].runNum == 0 || entry.score > rec.hall[i].score) {
            insertPos = i;
            break;
        }
    }

    if (insertPos >= 0) {
        for (int j = MAX_HALL_ENTRIES - 1; j > insertPos; --j) {
            rec.hall[j] = rec.hall[j - 1];
        }
        rec.hall[insertPos] = entry;
        res.hallRank = insertPos;
    } else {
        res.hallRank = -1;
    }
}

int countAchievements(uint32_t mask) {
    int c = 0;
    for (int i = 0; i < NUM_ACHIEVEMENTS; ++i) {
        if (mask & static_cast<uint32_t>(kAchievements[i].bit)) {
            ++c;
        }
    }
    return c;
}

} // namespace rogue
