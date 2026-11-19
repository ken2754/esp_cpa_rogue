#pragma once

#include "dungeon.hpp"
#include "records.hpp"
#include "types.hpp"
#include <array>
#include <random>
#include <string>
#include <vector>

namespace rogue {

struct LogMessage {
    std::string text;
    MsgColor color = MsgColor::Normal;
};

struct MonsterDef {
    char symbol;
    const char* name;
    int minFloor;
    int maxFloor;
    int baseHp;
    int ac;
    int dmgDice;
    int dmgSides;
    int xp;
    uint8_t tier; // 0=low, 1=mid, 2=high, 3=boss
};

const MonsterDef& getMonsterDef(MonsterId id);

class RogueGame {
public:
    RogueGame();

    void startNewGame(uint32_t seed, const RecordsData& records);
    void step(Action act);

    // Inventory actions (used both by shortcut keys and Inventory UI)
    bool useInventoryItem(int idx);
    bool dropInventoryItem(int idx);
    bool identifyInventoryItem(int idx);
    std::string getItemName(const Item& item) const;
    char getItemSymbol(ItemKind kind) const;

    // Accessors for renderer
    const DungeonMap& map() const { return map_; }
    const Player& player() const { return player_; }
    const std::vector<Item>& inventory() const { return inventory_; }
    const std::vector<Item>& groundItems() const { return groundItems_; }
    const std::array<Monster, MAX_MONSTERS>& monsters() const { return monsters_; }
    GameOutcome outcome() const { return outcome_; }
    const std::string& deathCause() const { return deathCause_; }
    int effectiveAc() const;
    int effectiveStr() const;
    int computeScore() const;
    uint32_t runAchievements() const { return runAchMask_; }

    // Camera top-left
    int camX() const;
    int camY() const;

    // Message queue
    bool hasMessages() const { return !msgQueue_.empty(); }
    const LogMessage& frontMessage() const { return msgQueue_.front(); }
    void popMessage();
    const std::vector<LogMessage>& messageHistory() const { return msgHistory_; }

    // Scroll of Identify targeting
    bool awaitingIdentify() const { return awaitingIdentify_; }
    void cancelIdentify() { awaitingIdentify_ = false; }

    // Build RunResult when game ends
    RunResult buildRunResult() const;

private:
    DungeonMap map_;
    Player player_;
    std::vector<Item> inventory_;
    std::vector<Item> groundItems_;
    std::array<Monster, MAX_MONSTERS> monsters_;
    std::mt19937 rng_;

    GameOutcome outcome_ = GameOutcome::Playing;
    std::string deathCause_;
    uint32_t runAchMask_ = 0;
    int identifiedCount_ = 0;
    int16_t prevBestFloor_ = 0;
    bool announcedFloorRecord_ = false;
    bool awaitingIdentify_ = false;

    // Per-run randomized appearance indices
    std::array<uint8_t, static_cast<size_t>(PotionId::COUNT)> potionAppear_{};
    std::array<uint8_t, static_cast<size_t>(ScrollId::COUNT)> scrollAppear_{};
    std::array<uint8_t, static_cast<size_t>(RingId::COUNT)> ringAppear_{};
    std::array<bool, static_cast<size_t>(PotionId::COUNT)> potionKnown_{};
    std::array<bool, static_cast<size_t>(ScrollId::COUNT)> scrollKnown_{};
    std::array<bool, static_cast<size_t>(RingId::COUNT)> ringKnown_{};

    std::vector<LogMessage> msgQueue_;
    std::vector<LogMessage> msgHistory_;

    void pushMsg(const std::string& text, MsgColor color = MsgColor::Normal);
    void initAppearances();
    void generateFloor(int floor, bool ascending);
    void spawnMonstersForFloor(int floor);
    void spawnItemsForFloor(int floor);
    Item createRandomItem(int floor);
    void updateFov();

    bool tryMoveOrAttack(int dx, int dy);
    void attackMonster(Monster& m);
    void monsterAttackPlayer(Monster& m);
    void updateMonsters();
    void endPlayerTurn(bool tookAction);

    void pickupAtPlayer();
    void triggerTrap(TrapCell& trap);
    void doSearch();
    void useStairs(bool goDown);
    void checkLevelUp();
    void killPlayer(const std::string& cause);
    void teleportPlayer();

    Monster* monsterAt(int x, int y);
    bool hasRing(RingId id) const;
    int rollDice(int count, int sides);
};

} // namespace rogue
