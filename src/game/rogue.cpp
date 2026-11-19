#include "rogue.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace rogue {

static const MonsterDef kMonsterDefs[static_cast<size_t>(MonsterId::COUNT)] = {
    {'B', "Bat",          1,  6,  4, 8, 1, 2,   2, 0},
    {'E', "Emu",          1,  5,  5, 7, 1, 3,   3, 0},
    {'K', "Kobold",       1,  6,  6, 7, 1, 4,   4, 0},
    {'S', "Rattlesnake",  2,  8,  9, 6, 1, 5,   9, 0},
    {'H', "Hobgoblin",    2,  8, 10, 6, 1, 6,  10, 1},
    {'O', "Orc",          3, 10, 12, 6, 1, 7,  14, 1},
    {'Z', "Zombie",       4, 11, 14, 7, 1, 8,  18, 1},
    {'I', "Ice Monster",  4, 11, 13, 6, 1, 4,  20, 1},
    {'C', "Centaur",      5, 13, 18, 5, 2, 5,  30, 1},
    {'R', "Rust Monster", 6, 14, 16, 5, 1, 4,  32, 2},
    {'W', "Wraith",       7, 16, 22, 4, 1, 8,  45, 2},
    {'T', "Troll",        8, 16, 28, 4, 2, 6,  65, 2},
    {'V', "Vampire",     10, 16, 34, 3, 2, 7,  90, 2},
    {'M', "Medusa",      11, 16, 38, 3, 2, 8, 120, 3},
    {'D', "Dragon",      13, 16, 55, 2, 3, 7, 220, 3},
    {'J', "Jabberwock",  14, 16, 65, 1, 3, 8, 300, 3},
};

const MonsterDef& getMonsterDef(MonsterId id) {
    return kMonsterDefs[static_cast<size_t>(id)];
}

static const char* kWeaponNames[] = {"Dagger", "Mace", "Long Sword", "Battle Axe", "2H Sword"};
static const int kWeaponDice[] = {1, 2, 2, 2, 3};
static const int kWeaponSides[] = {4, 4, 5, 6, 6};

static const char* kArmorNames[] = {"Leather", "Ring Mail", "Scale Mail", "Chain Mail", "Splint Mail", "Plate Mail"};
static const int kArmorBaseAc[] = {8, 7, 6, 5, 4, 3};

static const char* kPotionColors[] = {"red", "blue", "green", "amber", "violet", "clear", "murky", "silver", "crimson"};
static const char* kPotionNames[] = {
    "Healing", "Extra Healing", "Gain Strength", "Restore Str",
    "Poison", "Confusion", "Blindness", "Haste Self", "See Invis"
};

static const char* kScrollTitles[] = {
    "XYZZY", "ZELGO", "FOOBIE", "ELBERETH", "KLAATU", "JUJUBE", "ABRAXAS", "MORDOR"
};
static const char* kScrollNames[] = {
    "Identify", "Enchant Wpn", "Enchant Arm", "Teleport",
    "Magic Map", "Remove Curse", "Hold Monster", "Aggravate"
};

static const char* kRingGems[] = {"ruby", "sapphire", "emerald", "onyx", "opal"};
static const char* kRingNames[] = {
    "Protection", "Add Strength", "Sustain Str", "Regeneration", "Searching"
};

RogueGame::RogueGame() {
    rng_.seed(12345);
}

int RogueGame::rollDice(int count, int sides) {
    int sum = 0;
    for (int i = 0; i < count; ++i) {
        sum += std::uniform_int_distribution<int>(1, sides)(rng_);
    }
    return sum;
}

void RogueGame::pushMsg(const std::string& text, MsgColor color) {
    if (text.empty()) return;
    msgQueue_.push_back({text, color});
    msgHistory_.push_back({text, color});
    if (msgHistory_.size() > 24) {
        msgHistory_.erase(msgHistory_.begin());
    }
}

void RogueGame::popMessage() {
    if (!msgQueue_.empty()) {
        msgQueue_.erase(msgQueue_.begin());
    }
}

void RogueGame::initAppearances() {
    for (size_t i = 0; i < potionAppear_.size(); ++i) {
        potionAppear_[i] = static_cast<uint8_t>(i);
        potionKnown_[i] = false;
    }
    std::shuffle(potionAppear_.begin(), potionAppear_.end(), rng_);

    for (size_t i = 0; i < scrollAppear_.size(); ++i) {
        scrollAppear_[i] = static_cast<uint8_t>(i);
        scrollKnown_[i] = false;
    }
    std::shuffle(scrollAppear_.begin(), scrollAppear_.end(), rng_);

    for (size_t i = 0; i < ringAppear_.size(); ++i) {
        ringAppear_[i] = static_cast<uint8_t>(i);
        ringKnown_[i] = false;
    }
    std::shuffle(ringAppear_.begin(), ringAppear_.end(), rng_);
}

void RogueGame::startNewGame(uint32_t seed, const RecordsData& records) {
    rng_.seed(seed == 0 ? 1 : seed);
    player_ = Player{};
    inventory_.clear();
    groundItems_.clear();
    msgQueue_.clear();
    msgHistory_.clear();
    outcome_ = GameOutcome::Playing;
    deathCause_.clear();
    runAchMask_ = 0;
    identifiedCount_ = 0;
    prevBestFloor_ = records.bestFloor;
    announcedFloorRecord_ = false;
    awaitingIdentify_ = false;

    initAppearances();

    // Starting equipment: +1 Mace (wielded), Ring Mail (worn), 1 Food, 1 Healing Potion
    Item mace;
    mace.kind = ItemKind::Weapon;
    mace.subId = static_cast<uint8_t>(WeaponId::Mace);
    mace.plus = 1;
    mace.dmgPlus = 1;
    mace.equipped = true;
    mace.identified = true;
    inventory_.push_back(mace);
    player_.wieldedIdx = 0;

    Item armor;
    armor.kind = ItemKind::Armor;
    armor.subId = static_cast<uint8_t>(ArmorId::RingMail);
    armor.plus = 0;
    armor.equipped = true;
    armor.identified = true;
    inventory_.push_back(armor);
    player_.armorIdx = 1;

    Item food;
    food.kind = ItemKind::Food;
    food.count = 1;
    food.identified = true;
    inventory_.push_back(food);

    Item heal;
    heal.kind = ItemKind::Potion;
    heal.subId = static_cast<uint8_t>(PotionId::Healing);
    heal.count = 1;
    potionKnown_[static_cast<size_t>(PotionId::Healing)] = true;
    heal.identified = true;
    inventory_.push_back(heal);

    generateFloor(1, false);
    pushMsg("Welcome to Rogue! Find the Amulet!", MsgColor::Highlight);
}

void RogueGame::generateFloor(int floor, bool ascending) {
    player_.floor = static_cast<int16_t>(floor);
    if (floor > player_.maxFloorReached) {
        player_.maxFloorReached = static_cast<int16_t>(floor);
    }

    if (floor >= 5) runAchMask_ |= ACH_DEEP_DIVER;
    if (floor >= 10) runAchMask_ |= ACH_ABYSSAL;

    if (prevBestFloor_ > 0 && floor > prevBestFloor_ && !announcedFloorRecord_) {
        announcedFloorRecord_ = true;
        char buf[48];
        std::snprintf(buf, sizeof(buf), "NEW RECORD! Deepest floor B%dF!", floor);
        pushMsg(buf, MsgColor::Highlight);
    }

    generateDungeon(map_, floor, ascending, rng_);
    player_.x = static_cast<int16_t>(map_.startX);
    player_.y = static_cast<int16_t>(map_.startY);

    groundItems_.clear();
    for (auto& m : monsters_) m = Monster{};

    spawnMonstersForFloor(floor);
    spawnItemsForFloor(floor);

    // Spawn Amulet of Yendor on AMULET_FLOOR if player doesn't have it yet
    if (floor == AMULET_FLOOR && !player_.hasAmulet) {
        for (int tries = 0; tries < 50; ++tries) {
            int rx = std::uniform_int_distribution<int>(1, MAP_W - 2)(rng_);
            int ry = std::uniform_int_distribution<int>(1, MAP_H - 2)(rng_);
            if (map_.tiles[ry][rx] == Tile::Floor && !(rx == player_.x && ry == player_.y)) {
                Item amu;
                amu.kind = ItemKind::Amulet;
                amu.identified = true;
                amu.x = static_cast<int16_t>(rx);
                amu.y = static_cast<int16_t>(ry);
                groundItems_.push_back(amu);
                break;
            }
        }
    }

    updateFov();
}

void RogueGame::spawnMonstersForFloor(int floor) {
    std::vector<MonsterId> pool;
    for (size_t i = 0; i < static_cast<size_t>(MonsterId::COUNT); ++i) {
        if (floor >= kMonsterDefs[i].minFloor && floor <= kMonsterDefs[i].maxFloor) {
            pool.push_back(static_cast<MonsterId>(i));
        }
    }
    if (pool.empty()) pool.push_back(MonsterId::Bat);

    int targetCount = std::min(MAX_MONSTERS, 5 + floor + std::uniform_int_distribution<int>(0, 3)(rng_));
    int spawned = 0;
    for (int tries = 0; tries < 200 && spawned < targetCount; ++tries) {
        int rx = std::uniform_int_distribution<int>(1, MAP_W - 2)(rng_);
        int ry = std::uniform_int_distribution<int>(1, MAP_H - 2)(rng_);
        if (map_.tiles[ry][rx] != Tile::Floor) continue;
        if (std::abs(rx - player_.x) <= 3 && std::abs(ry - player_.y) <= 3) continue;
        if (monsterAt(rx, ry) != nullptr) continue;

        MonsterId id = pool[std::uniform_int_distribution<int>(0, static_cast<int>(pool.size()) - 1)(rng_)];
        const MonsterDef& def = getMonsterDef(id);
        Monster& m = monsters_[spawned++];
        m.id = id;
        m.x = static_cast<int16_t>(rx);
        m.y = static_cast<int16_t>(ry);
        int hpVar = std::max(1, def.baseHp / 4);
        m.maxHp = static_cast<int16_t>(def.baseHp + std::uniform_int_distribution<int>(-hpVar, hpVar)(rng_));
        m.hp = m.maxHp;
        m.ac = static_cast<int8_t>(def.ac);
        m.awake = (map_.roomAt(rx, ry) == map_.roomAt(player_.x, player_.y)) && (std::uniform_int_distribution<int>(0, 1)(rng_) == 0);
        m.held = false;
        m.invisible = false;
        m.alive = true;
    }
}

Item RogueGame::createRandomItem(int floor) {
    Item it;
    int roll = std::uniform_int_distribution<int>(0, 99)(rng_);
    if (roll < 24) {
        it.kind = ItemKind::Food;
        it.identified = true;
    } else if (roll < 52) {
        it.kind = ItemKind::Potion;
        it.subId = static_cast<uint8_t>(std::uniform_int_distribution<int>(0, static_cast<int>(PotionId::COUNT) - 1)(rng_));
        it.identified = potionKnown_[it.subId];
    } else if (roll < 76) {
        it.kind = ItemKind::Scroll;
        it.subId = static_cast<uint8_t>(std::uniform_int_distribution<int>(0, static_cast<int>(ScrollId::COUNT) - 1)(rng_));
        it.identified = scrollKnown_[it.subId];
    } else if (roll < 87) {
        it.kind = ItemKind::Weapon;
        int maxW = std::min(static_cast<int>(WeaponId::COUNT) - 1, 1 + floor / 3);
        it.subId = static_cast<uint8_t>(std::uniform_int_distribution<int>(0, maxW)(rng_));
        int ench = std::uniform_int_distribution<int>(0, 99)(rng_);
        if (ench < 12) { it.plus = -1; it.cursed = true; }
        else if (ench < 35) { it.plus = 1; it.dmgPlus = 1; }
        else if (ench < 45 && floor >= 5) { it.plus = 2; it.dmgPlus = 2; }
    } else if (roll < 96) {
        it.kind = ItemKind::Armor;
        int maxA = std::min(static_cast<int>(ArmorId::COUNT) - 1, 1 + floor / 3);
        it.subId = static_cast<uint8_t>(std::uniform_int_distribution<int>(0, maxA)(rng_));
        int ench = std::uniform_int_distribution<int>(0, 99)(rng_);
        if (ench < 12) { it.plus = -1; it.cursed = true; }
        else if (ench < 35) { it.plus = 1; }
        else if (ench < 45 && floor >= 5) { it.plus = 2; }
    } else {
        it.kind = ItemKind::Ring;
        it.subId = static_cast<uint8_t>(std::uniform_int_distribution<int>(0, static_cast<int>(RingId::COUNT) - 1)(rng_));
        it.identified = ringKnown_[it.subId];
        if (it.subId == static_cast<uint8_t>(RingId::Protection) || it.subId == static_cast<uint8_t>(RingId::AddStrength)) {
            it.plus = (std::uniform_int_distribution<int>(0, 4)(rng_) == 0) ? -1 : 1;
            if (it.plus < 0) it.cursed = true;
        }
    }
    return it;
}

void RogueGame::spawnItemsForFloor(int floor) {
    int count = 4 + std::uniform_int_distribution<int>(0, 3)(rng_);
    for (int i = 0; i < count && static_cast<int>(groundItems_.size()) < MAX_GROUND_ITEMS; ++i) {
        for (int tries = 0; tries < 40; ++tries) {
            int rx = std::uniform_int_distribution<int>(1, MAP_W - 2)(rng_);
            int ry = std::uniform_int_distribution<int>(1, MAP_H - 2)(rng_);
            if (map_.tiles[ry][rx] == Tile::Floor && !(rx == player_.x && ry == player_.y)) {
                Item it = createRandomItem(floor);
                it.x = static_cast<int16_t>(rx);
                it.y = static_cast<int16_t>(ry);
                groundItems_.push_back(it);
                break;
            }
        }
    }
}

void RogueGame::updateFov() {
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            map_.visible[y][x] = false;
        }
    }

    if (player_.blindTurns > 0) {
        map_.visible[player_.y][player_.x] = true;
        map_.explored[player_.y][player_.x] = true;
        return;
    }

    // Always see 3x3 around player
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int nx = player_.x + dx;
            int ny = player_.y + dy;
            if (map_.inBounds(nx, ny) && map_.tiles[ny][nx] != Tile::Void) {
                map_.visible[ny][nx] = true;
                map_.explored[ny][nx] = true;
            }
        }
    }

    // If in a lit room, reveal the whole room
    int rIdx = map_.roomAt(player_.x, player_.y);
    if (rIdx >= 0 && !map_.rooms[rIdx].dark) {
        const Room& r = map_.rooms[rIdx];
        for (int y = r.y; y < r.y + r.h; ++y) {
            for (int x = r.x; x < r.x + r.w; ++x) {
                map_.visible[y][x] = true;
                map_.explored[y][x] = true;
            }
        }
    }
}

Monster* RogueGame::monsterAt(int x, int y) {
    for (auto& m : monsters_) {
        if (m.alive && m.x == x && m.y == y) return &m;
    }
    return nullptr;
}

bool RogueGame::hasRing(RingId id) const {
    if (player_.ringIdx < 0 || player_.ringIdx >= static_cast<int>(inventory_.size())) return false;
    const Item& r = inventory_[player_.ringIdx];
    return r.kind == ItemKind::Ring && r.subId == static_cast<uint8_t>(id);
}

int RogueGame::effectiveAc() const {
    int ac = player_.baseAc;
    if (player_.armorIdx >= 0 && player_.armorIdx < static_cast<int>(inventory_.size())) {
        const Item& arm = inventory_[player_.armorIdx];
        ac = kArmorBaseAc[arm.subId] - arm.plus;
    }
    if (hasRing(RingId::Protection)) {
        ac -= inventory_[player_.ringIdx].plus;
    }
    return std::max(-5, ac);
}

int RogueGame::effectiveStr() const {
    int s = player_.str;
    if (hasRing(RingId::AddStrength)) {
        s += inventory_[player_.ringIdx].plus;
    }
    return std::max(3, s);
}

int RogueGame::computeScore() const {
    int s = player_.gold + player_.kills * 15 + (player_.maxFloorReached - 1) * 100;
    if (player_.hasAmulet) s += 1000;
    if (outcome_ == GameOutcome::Escaped) s += 2500 + player_.hp * 10;
    return s;
}

int RogueGame::camX() const {
    int cx = player_.x - VIEW_W / 2;
    return std::clamp(cx, 0, MAP_W - VIEW_W);
}

int RogueGame::camY() const {
    int cy = player_.y - VIEW_H / 2;
    return std::clamp(cy, 0, MAP_H - VIEW_H);
}

char RogueGame::getItemSymbol(ItemKind kind) const {
    switch (kind) {
        case ItemKind::Food:   return ':';
        case ItemKind::Weapon: return ')';
        case ItemKind::Armor:  return ']';
        case ItemKind::Potion: return '!';
        case ItemKind::Scroll: return '?';
        case ItemKind::Ring:   return '=';
        case ItemKind::Amulet: return ',';
    }
    return '?';
}

std::string RogueGame::getItemName(const Item& item) const {
    char buf[48];
    switch (item.kind) {
        case ItemKind::Food:
            return item.count > 1 ? ("Food x" + std::to_string(item.count)) : "Food Ration";
        case ItemKind::Weapon: {
            const char* wn = kWeaponNames[item.subId];
            if (item.identified) {
                std::snprintf(buf, sizeof(buf), "%+d %s", item.plus, wn);
                return buf;
            }
            return wn;
        }
        case ItemKind::Armor: {
            const char* an = kArmorNames[item.subId];
            if (item.identified) {
                std::snprintf(buf, sizeof(buf), "%+d %s", item.plus, an);
                return buf;
            }
            return an;
        }
        case ItemKind::Potion: {
            bool known = item.identified || potionKnown_[item.subId];
            if (known) {
                std::snprintf(buf, sizeof(buf), "Pot:%s", kPotionNames[item.subId]);
            } else {
                std::snprintf(buf, sizeof(buf), "%s potion", kPotionColors[potionAppear_[item.subId]]);
            }
            std::string s(buf);
            if (item.count > 1) s += " x" + std::to_string(item.count);
            return s;
        }
        case ItemKind::Scroll: {
            bool known = item.identified || scrollKnown_[item.subId];
            if (known) {
                std::snprintf(buf, sizeof(buf), "Scr:%s", kScrollNames[item.subId]);
            } else {
                std::snprintf(buf, sizeof(buf), "Scr '%s'", kScrollTitles[scrollAppear_[item.subId]]);
            }
            std::string s(buf);
            if (item.count > 1) s += " x" + std::to_string(item.count);
            return s;
        }
        case ItemKind::Ring: {
            bool known = item.identified || ringKnown_[item.subId];
            if (known) {
                if (item.subId == static_cast<uint8_t>(RingId::Protection) ||
                    item.subId == static_cast<uint8_t>(RingId::AddStrength)) {
                    std::snprintf(buf, sizeof(buf), "%+d Ring:%s", item.plus, kRingNames[item.subId]);
                } else {
                    std::snprintf(buf, sizeof(buf), "Ring:%s", kRingNames[item.subId]);
                }
            } else {
                std::snprintf(buf, sizeof(buf), "%s ring", kRingGems[ringAppear_[item.subId]]);
            }
            return buf;
        }
        case ItemKind::Amulet:
            return "Amulet of Yendor";
    }
    return "Unknown";
}

void RogueGame::step(Action act) {
    if (outcome_ != GameOutcome::Playing) return;

    // If frozen or trapped in bear trap, movement/actions consume turn
    if (player_.frozenTurns > 0) {
        player_.frozenTurns--;
        pushMsg("You are frozen!", MsgColor::Warn);
        endPlayerTurn(true);
        return;
    }

    int dx = 0, dy = 0;
    bool isMove = false;

    switch (act) {
        case Action::MoveN:  dy = -1; isMove = true; break;
        case Action::MoveS:  dy =  1; isMove = true; break;
        case Action::MoveW:  dx = -1; isMove = true; break;
        case Action::MoveE:  dx =  1; isMove = true; break;
        case Action::MoveNW: dx = -1; dy = -1; isMove = true; break;
        case Action::MoveNE: dx =  1; dy = -1; isMove = true; break;
        case Action::MoveSW: dx = -1; dy =  1; isMove = true; break;
        case Action::MoveSE: dx =  1; dy =  1; isMove = true; break;
        case Action::Rest:
            endPlayerTurn(true);
            return;
        case Action::Search:
            doSearch();
            endPlayerTurn(true);
            return;
        case Action::Descend:
            useStairs(true);
            return;
        case Action::Ascend:
            useStairs(false);
            return;
        case Action::UseStairs: {
            Tile t = map_.tiles[player_.y][player_.x];
            if (t == Tile::StairsDown) useStairs(true);
            else if (t == Tile::StairsUp) useStairs(false);
            else pushMsg("No stairs here.", MsgColor::Normal);
            return;
        }
        default:
            return;
    }

    if (isMove) {
        if (player_.bearTrapTurns > 0) {
            player_.bearTrapTurns--;
            pushMsg("Stuck in a bear trap!", MsgColor::Warn);
            endPlayerTurn(true);
            return;
        }
        if (player_.confusedTurns > 0) {
            static const int dxs[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
            static const int dys[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
            int dir = std::uniform_int_distribution<int>(0, 7)(rng_);
            dx = dxs[dir];
            dy = dys[dir];
        }
        if (tryMoveOrAttack(dx, dy)) {
            endPlayerTurn(true);
        }
    }
}

bool RogueGame::tryMoveOrAttack(int dx, int dy) {
    int nx = player_.x + dx;
    int ny = player_.y + dy;
    if (!map_.inBounds(nx, ny)) return false;

    if (Monster* m = monsterAt(nx, ny)) {
        attackMonster(*m);
        return true;
    }

    if (!map_.isWalkable(nx, ny)) {
        return false;
    }

    player_.x = static_cast<int16_t>(nx);
    player_.y = static_cast<int16_t>(ny);
    updateFov();

    // Check gold
    for (int i = 0; i < MAX_ROOMS; ++i) {
        if (map_.gold[i].exists && map_.gold[i].x == nx && map_.gold[i].y == ny) {
            player_.gold += map_.gold[i].amount;
            char buf[40];
            std::snprintf(buf, sizeof(buf), "Found %d gold!", map_.gold[i].amount);
            pushMsg(buf, MsgColor::Highlight);
            map_.gold[i].exists = false;
            if (player_.gold >= 500) runAchMask_ |= ACH_HOARDER;
        }
    }

    // Check items
    pickupAtPlayer();

    // Check traps
    if (map_.tiles[ny][nx] == Tile::Trap) {
        for (int i = 0; i < map_.trapCount; ++i) {
            if (map_.traps[i].exists && map_.traps[i].x == nx && map_.traps[i].y == ny) {
                triggerTrap(map_.traps[i]);
                break;
            }
        }
    }

    return true;
}

void RogueGame::pickupAtPlayer() {
    for (size_t i = 0; i < groundItems_.size(); ++i) {
        Item& it = groundItems_[i];
        if (it.x == player_.x && it.y == player_.y) {
            if (it.kind == ItemKind::Amulet) {
                player_.hasAmulet = true;
                runAchMask_ |= ACH_AMULET_BEARER;
                pushMsg("You found the Amulet of Yendor!", MsgColor::Highlight);
                groundItems_.erase(groundItems_.begin() + i);
                // Convert stairs down to stairs up on current floor!
                if (map_.tiles[map_.stairsDownY][map_.stairsDownX] == Tile::StairsDown) {
                    map_.tiles[map_.stairsDownY][map_.stairsDownX] = Tile::StairsUp;
                    map_.stairsUpX = map_.stairsDownX;
                    map_.stairsUpY = map_.stairsDownY;
                }
                return;
            }

            // Stack food, potions, scrolls
            if (it.kind == ItemKind::Food || it.kind == ItemKind::Potion || it.kind == ItemKind::Scroll) {
                for (auto& inv : inventory_) {
                    if (inv.kind == it.kind && (it.kind == ItemKind::Food || inv.subId == it.subId)) {
                        inv.count += it.count;
                        pushMsg("Got " + getItemName(it), MsgColor::Good);
                        groundItems_.erase(groundItems_.begin() + i);
                        return;
                    }
                }
            }

            if (static_cast<int>(inventory_.size()) >= MAX_INVENTORY) {
                pushMsg("Pack is full!", MsgColor::Warn);
                return;
            }

            std::string name = getItemName(it);
            inventory_.push_back(it);
            groundItems_.erase(groundItems_.begin() + i);
            pushMsg("Got " + name, MsgColor::Good);
            return;
        }
    }
}

void RogueGame::triggerTrap(TrapCell& trap) {
    trap.discovered = true;
    switch (trap.kind) {
        case TrapKind::BearTrap:
            player_.bearTrapTurns = 2;
            pushMsg("Caught in a bear trap!", MsgColor::Warn);
            break;
        case TrapKind::DartTrap: {
            int dmg = rollDice(1, 4);
            player_.hp -= dmg;
            pushMsg("A dart hits you!", MsgColor::Danger);
            if (player_.hp <= 0) killPlayer("Killed by a dart trap");
            break;
        }
        case TrapKind::TeleportTrap:
            pushMsg("Teleport trap!", MsgColor::Warn);
            teleportPlayer();
            break;
        case TrapKind::SleepGas:
            player_.frozenTurns = 2;
            pushMsg("Sleep gas! You pass out.", MsgColor::Warn);
            break;
        case TrapKind::Pit: {
            int dmg = rollDice(1, 6);
            player_.hp -= dmg;
            pushMsg("You fell into a pit!", MsgColor::Danger);
            if (player_.hp <= 0) killPlayer("Fell into a pit");
            break;
        }
    }
}

void RogueGame::doSearch() {
    bool found = false;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int nx = player_.x + dx;
            int ny = player_.y + dy;
            if (!map_.inBounds(nx, ny)) continue;
            if (map_.tiles[ny][nx] == Tile::SecretDoor) {
                map_.tiles[ny][nx] = Tile::Door;
                found = true;
                pushMsg("Found a secret door!", MsgColor::Good);
            } else if (map_.tiles[ny][nx] == Tile::Trap) {
                for (int i = 0; i < map_.trapCount; ++i) {
                    if (map_.traps[i].exists && map_.traps[i].x == nx && map_.traps[i].y == ny && !map_.traps[i].discovered) {
                        map_.traps[i].discovered = true;
                        found = true;
                        pushMsg("Found a trap!", MsgColor::Warn);
                    }
                }
            }
        }
    }
    if (!found) {
        pushMsg("You search around.", MsgColor::Normal);
    }
}

void RogueGame::useStairs(bool goDown) {
    Tile t = map_.tiles[player_.y][player_.x];
    if (goDown && t == Tile::StairsDown) {
        int nextFloor = player_.floor + 1;
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Descended to B%dF.", nextFloor);
        pushMsg(buf, MsgColor::Normal);
        generateFloor(nextFloor, false);
    } else if (!goDown && t == Tile::StairsUp) {
        if (player_.floor == 1 && player_.hasAmulet) {
            outcome_ = GameOutcome::Escaped;
            runAchMask_ |= ACH_TRUE_ROGUE;
            deathCause_ = "Escaped with Amulet!";
            pushMsg("You escaped with the Amulet!", MsgColor::Highlight);
            return;
        }
        int prevFloor = std::max(1, player_.floor - 1);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Ascended to B%dF.", prevFloor);
        pushMsg(buf, MsgColor::Normal);
        generateFloor(prevFloor, true);
    } else {
        pushMsg("No matching stairs here.", MsgColor::Normal);
    }
}

void RogueGame::attackMonster(Monster& m) {
    m.awake = true;
    m.held = false;
    const MonsterDef& def = getMonsterDef(m.id);

    int wpnHit = 0;
    int dice = 1, sides = 3, wpnDmg = 0;
    if (player_.wieldedIdx >= 0 && player_.wieldedIdx < static_cast<int>(inventory_.size())) {
        const Item& w = inventory_[player_.wieldedIdx];
        wpnHit = w.plus;
        wpnDmg = w.dmgPlus;
        dice = kWeaponDice[w.subId];
        sides = kWeaponSides[w.subId];
    }

    int strHit = (effectiveStr() - 14) / 2;
    int need = 20 - player_.level - wpnHit - strHit - (10 - m.ac);
    int roll = std::uniform_int_distribution<int>(1, 20)(rng_);

    if (roll < need && roll != 20) {
        pushMsg(std::string("You miss the ") + def.name + ".", MsgColor::Normal);
        return;
    }

    int dmg = std::max(1, rollDice(dice, sides) + wpnDmg + (effectiveStr() - 15) / 2);
    m.hp -= dmg;
    if (m.hp <= 0) {
        m.alive = false;
        player_.kills++;
        player_.xp += def.xp;
        if (player_.kills >= 10) runAchMask_ |= ACH_FIRST_BLOOD;
        if (m.id == MonsterId::Dragon) runAchMask_ |= ACH_DRAGON_SLAYER;
        pushMsg(std::string("Defeated ") + def.name + "!", MsgColor::Good);
        checkLevelUp();
    } else {
        char buf[44];
        std::snprintf(buf, sizeof(buf), "Hit %s for %d!", def.name, dmg);
        pushMsg(buf, MsgColor::Normal);
    }
}

void RogueGame::monsterAttackPlayer(Monster& m) {
    const MonsterDef& def = getMonsterDef(m.id);
    int need = 20 - std::max(1, def.minFloor + 1) - (10 - effectiveAc());
    int roll = std::uniform_int_distribution<int>(1, 20)(rng_);
    if (roll < need && roll != 20) {
        return; // Monster misses silently to keep log clean
    }

    int dmg = rollDice(def.dmgDice, def.dmgSides);
    // Dragon breath bonus
    if (m.id == MonsterId::Dragon && std::uniform_int_distribution<int>(0, 3)(rng_) == 0) {
        dmg += rollDice(1, 6);
        pushMsg("Dragon breathes fire!", MsgColor::Danger);
    }

    player_.hp -= dmg;
    char buf[44];
    std::snprintf(buf, sizeof(buf), "%s hits you (-%d HP)", def.name, dmg);
    pushMsg(buf, MsgColor::Danger);

    if (player_.hp <= 0) {
        killPlayer(std::string("Killed by ") + def.name);
        return;
    }

    // Special monster effects
    if (m.id == MonsterId::RustMonster && player_.armorIdx >= 0) {
        Item& arm = inventory_[player_.armorIdx];
        if (arm.subId != static_cast<uint8_t>(ArmorId::Leather) &&
            std::uniform_int_distribution<int>(0, 2)(rng_) == 0) {
            arm.plus--;
            pushMsg("Your armor rusts!", MsgColor::Warn);
        }
    } else if (m.id == MonsterId::IceMonster && std::uniform_int_distribution<int>(0, 2)(rng_) == 0) {
        player_.frozenTurns = 1;
        pushMsg("Ice Monster chills you!", MsgColor::Warn);
    } else if (m.id == MonsterId::Rattlesnake && !hasRing(RingId::SustainStrength) &&
               std::uniform_int_distribution<int>(0, 3)(rng_) == 0) {
        if (player_.str > 3) {
            player_.str--;
            pushMsg("Snake bite weakens you!", MsgColor::Warn);
        }
    } else if (m.id == MonsterId::Wraith && std::uniform_int_distribution<int>(0, 3)(rng_) == 0) {
        if (player_.maxHp > 5) {
            player_.maxHp--;
            player_.hp = std::min(player_.hp, player_.maxHp);
            pushMsg("Wraith drains your life!", MsgColor::Danger);
        }
    } else if (m.id == MonsterId::Medusa && std::uniform_int_distribution<int>(0, 2)(rng_) == 0) {
        player_.confusedTurns = 4;
        pushMsg("Medusa's gaze confuses you!", MsgColor::Warn);
    }
}

void RogueGame::updateMonsters() {
    int pRoom = map_.roomAt(player_.x, player_.y);
    for (auto& m : monsters_) {
        if (!m.alive) continue;

        if (m.id == MonsterId::Troll && player_.turns % 3 == 0 && m.hp < m.maxHp) {
            m.hp++;
        }

        if (m.held) continue;

        int dist = std::max(std::abs(m.x - player_.x), std::abs(m.y - player_.y));
        if (!m.awake) {
            int mRoom = map_.roomAt(m.x, m.y);
            if (dist <= 2 || (pRoom >= 0 && mRoom == pRoom)) {
                m.awake = true;
            } else {
                continue;
            }
        }

        if (dist == 1) {
            monsterAttackPlayer(m);
            if (outcome_ != GameOutcome::Playing) return;
            continue;
        }

        if (dist > 14) continue;

        int stepX = 0, stepY = 0;
        if (m.id == MonsterId::Bat && std::uniform_int_distribution<int>(0, 1)(rng_) == 0) {
            stepX = std::uniform_int_distribution<int>(-1, 1)(rng_);
            stepY = std::uniform_int_distribution<int>(-1, 1)(rng_);
        } else {
            if (player_.x > m.x) stepX = 1;
            else if (player_.x < m.x) stepX = -1;
            if (player_.y > m.y) stepY = 1;
            else if (player_.y < m.y) stepY = -1;
        }

        int candidates[3][2] = {{stepX, stepY}, {stepX, 0}, {0, stepY}};
        for (int c = 0; c < 3; ++c) {
            int nx = m.x + candidates[c][0];
            int ny = m.y + candidates[c][1];
            if (candidates[c][0] == 0 && candidates[c][1] == 0) continue;
            if (nx == player_.x && ny == player_.y) continue;
            if (map_.isWalkable(nx, ny) && monsterAt(nx, ny) == nullptr) {
                m.x = static_cast<int16_t>(nx);
                m.y = static_cast<int16_t>(ny);
                break;
            }
        }
    }
}

void RogueGame::endPlayerTurn(bool tookAction) {
    if (!tookAction || outcome_ != GameOutcome::Playing) return;

    player_.turns++;
    if (player_.turns >= 1000) runAchMask_ |= ACH_SURVIVOR;

    if (player_.confusedTurns > 0) player_.confusedTurns--;
    if (player_.blindTurns > 0) {
        player_.blindTurns--;
        if (player_.blindTurns == 0) updateFov();
    }

    // Haste gives player a free turn every other turn (monsters only move on even haste or normal)
    bool monsterTurn = true;
    if (player_.hasteTurns > 0) {
        player_.hasteTurns--;
        if (player_.turns % 2 == 1) monsterTurn = false;
    }

    // Natural HP regeneration
    int regenRate = hasRing(RingId::Regeneration) ? 4 : std::max(5, 16 - player_.level);
    if (player_.hp < player_.maxHp && player_.hunger > 150 && (player_.turns % regenRate == 0)) {
        player_.hp++;
    }

    // Ring of Searching
    if (hasRing(RingId::Searching) && (player_.turns % 3 == 0)) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int nx = player_.x + dx, ny = player_.y + dy;
                if (map_.inBounds(nx, ny) && map_.tiles[ny][nx] == Tile::SecretDoor) {
                    map_.tiles[ny][nx] = Tile::Door;
                    pushMsg("Ring reveals secret door!", MsgColor::Good);
                }
            }
        }
    }

    // Hunger
    int prevHunger = player_.hunger;
    player_.hunger--;
    if (prevHunger > 300 && player_.hunger <= 300) {
        pushMsg("You are getting hungry.", MsgColor::Warn);
    } else if (prevHunger > 150 && player_.hunger <= 150) {
        pushMsg("You feel weak from hunger!", MsgColor::Warn);
    } else if (prevHunger > 0 && player_.hunger <= 0) {
        pushMsg("You are fainting!", MsgColor::Danger);
    } else if (player_.hunger < -120) {
        killPlayer("Starved to death");
        return;
    }

    if (monsterTurn) {
        updateMonsters();
    }
}

void RogueGame::checkLevelUp() {
    while (player_.xp >= player_.nextXp) {
        player_.level++;
        player_.nextXp *= 2;
        int hpGain = rollDice(1, 8) + 2;
        player_.maxHp += hpGain;
        player_.hp += hpGain;
        if (player_.level >= 7) runAchMask_ |= ACH_VETERAN;
        char buf[40];
        std::snprintf(buf, sizeof(buf), "Welcome to Level %d!", player_.level);
        pushMsg(buf, MsgColor::Highlight);
    }
}

void RogueGame::killPlayer(const std::string& cause) {
    player_.hp = 0;
    outcome_ = GameOutcome::Dead;
    deathCause_ = cause;
}

void RogueGame::teleportPlayer() {
    for (int tries = 0; tries < 100; ++tries) {
        int rx = std::uniform_int_distribution<int>(1, MAP_W - 2)(rng_);
        int ry = std::uniform_int_distribution<int>(1, MAP_H - 2)(rng_);
        if (map_.tiles[ry][rx] == Tile::Floor && monsterAt(rx, ry) == nullptr) {
            player_.x = static_cast<int16_t>(rx);
            player_.y = static_cast<int16_t>(ry);
            updateFov();
            return;
        }
    }
}

bool RogueGame::identifyInventoryItem(int idx) {
    awaitingIdentify_ = false;
    if (idx < 0 || idx >= static_cast<int>(inventory_.size())) return false;
    Item& it = inventory_[idx];
    if (!it.identified) {
        it.identified = true;
        identifiedCount_++;
        if (identifiedCount_ >= 5) runAchMask_ |= ACH_SCHOLAR;
    }
    if (it.kind == ItemKind::Potion) potionKnown_[it.subId] = true;
    else if (it.kind == ItemKind::Scroll) scrollKnown_[it.subId] = true;
    else if (it.kind == ItemKind::Ring) ringKnown_[it.subId] = true;
    pushMsg("Identified: " + getItemName(it), MsgColor::Good);
    return true;
}

bool RogueGame::useInventoryItem(int idx) {
    if (idx < 0 || idx >= static_cast<int>(inventory_.size())) return false;
    Item& it = inventory_[idx];

    switch (it.kind) {
        case ItemKind::Food: {
            player_.hunger = std::min<int16_t>(1400, static_cast<int16_t>(player_.hunger + 850));
            pushMsg("Yum! That tasted good.", MsgColor::Good);
            if (it.count > 1) {
                it.count--;
            } else {
                if (player_.wieldedIdx > idx) player_.wieldedIdx--;
                if (player_.armorIdx > idx) player_.armorIdx--;
                if (player_.ringIdx > idx) player_.ringIdx--;
                inventory_.erase(inventory_.begin() + idx);
            }
            endPlayerTurn(true);
            return true;
        }
        case ItemKind::Weapon: {
            if (player_.wieldedIdx == idx) {
                if (it.cursed) {
                    pushMsg("It's cursed! Can't unwield.", MsgColor::Warn);
                    return false;
                }
                it.equipped = false;
                player_.wieldedIdx = -1;
                pushMsg("Unwielded " + getItemName(it), MsgColor::Normal);
            } else {
                if (player_.wieldedIdx >= 0 && inventory_[player_.wieldedIdx].cursed) {
                    pushMsg("Current weapon is cursed!", MsgColor::Warn);
                    return false;
                }
                if (player_.wieldedIdx >= 0) inventory_[player_.wieldedIdx].equipped = false;
                it.equipped = true;
                player_.wieldedIdx = static_cast<int8_t>(idx);
                pushMsg("Wielding " + getItemName(it), MsgColor::Good);
            }
            endPlayerTurn(true);
            return true;
        }
        case ItemKind::Armor: {
            if (player_.armorIdx == idx) {
                if (it.cursed) {
                    pushMsg("It's cursed! Can't remove.", MsgColor::Warn);
                    return false;
                }
                it.equipped = false;
                player_.armorIdx = -1;
                pushMsg("Removed " + getItemName(it), MsgColor::Normal);
            } else {
                if (player_.armorIdx >= 0 && inventory_[player_.armorIdx].cursed) {
                    pushMsg("Current armor is cursed!", MsgColor::Warn);
                    return false;
                }
                if (player_.armorIdx >= 0) inventory_[player_.armorIdx].equipped = false;
                it.equipped = true;
                player_.armorIdx = static_cast<int8_t>(idx);
                pushMsg("Wearing " + getItemName(it), MsgColor::Good);
            }
            endPlayerTurn(true);
            return true;
        }
        case ItemKind::Ring: {
            if (player_.ringIdx == idx) {
                if (it.cursed) {
                    pushMsg("It's cursed! Can't remove.", MsgColor::Warn);
                    return false;
                }
                it.equipped = false;
                player_.ringIdx = -1;
                pushMsg("Removed " + getItemName(it), MsgColor::Normal);
            } else {
                if (player_.ringIdx >= 0 && inventory_[player_.ringIdx].cursed) {
                    pushMsg("Current ring is cursed!", MsgColor::Warn);
                    return false;
                }
                if (player_.ringIdx >= 0) inventory_[player_.ringIdx].equipped = false;
                it.equipped = true;
                player_.ringIdx = static_cast<int8_t>(idx);
                pushMsg("Put on " + getItemName(it), MsgColor::Good);
            }
            endPlayerTurn(true);
            return true;
        }
        case ItemKind::Potion: {
            PotionId pid = static_cast<PotionId>(it.subId);
            if (!potionKnown_[it.subId]) {
                potionKnown_[it.subId] = true;
                identifiedCount_++;
                if (identifiedCount_ >= 5) runAchMask_ |= ACH_SCHOLAR;
            }
            switch (pid) {
                case PotionId::Healing:
                    player_.hp = std::min<int16_t>(player_.maxHp, static_cast<int16_t>(player_.hp + rollDice(2, 6) + 4));
                    pushMsg("You feel better!", MsgColor::Good);
                    break;
                case PotionId::ExtraHealing:
                    player_.maxHp += 2;
                    player_.hp = player_.maxHp;
                    pushMsg("Full healing! Max HP +2!", MsgColor::Highlight);
                    break;
                case PotionId::GainStrength:
                    player_.str++;
                    player_.maxStr = std::max(player_.maxStr, player_.str);
                    pushMsg("You feel stronger! Str +1", MsgColor::Good);
                    break;
                case PotionId::RestoreStrength:
                    player_.str = player_.maxStr;
                    pushMsg("Strength restored!", MsgColor::Good);
                    break;
                case PotionId::Poison:
                    player_.hp -= rollDice(1, 6);
                    if (player_.str > 3 && !hasRing(RingId::SustainStrength)) player_.str--;
                    pushMsg("Ugh! Poison!", MsgColor::Danger);
                    if (player_.hp <= 0) killPlayer("Poisoned by a potion");
                    break;
                case PotionId::Confusion:
                    player_.confusedTurns = 10;
                    pushMsg("Wait, what? You're confused!", MsgColor::Warn);
                    break;
                case PotionId::Blindness:
                    player_.blindTurns = 12;
                    updateFov();
                    pushMsg("A cloak of darkness falls!", MsgColor::Warn);
                    break;
                case PotionId::Haste:
                    player_.hasteTurns = 16;
                    pushMsg("You feel yourself speeding up!", MsgColor::Good);
                    break;
                case PotionId::SeeInvisible:
                    player_.seeInvisible = true;
                    pushMsg("Your eyes tingle!", MsgColor::Good);
                    break;
                default:
                    break;
            }
            if (it.count > 1) it.count--;
            else {
                // Adjust equipped indices if erasing before them
                if (player_.wieldedIdx > idx) player_.wieldedIdx--;
                if (player_.armorIdx > idx) player_.armorIdx--;
                if (player_.ringIdx > idx) player_.ringIdx--;
                inventory_.erase(inventory_.begin() + idx);
            }
            endPlayerTurn(true);
            return true;
        }
        case ItemKind::Scroll: {
            ScrollId sid = static_cast<ScrollId>(it.subId);
            if (!scrollKnown_[it.subId]) {
                scrollKnown_[it.subId] = true;
                identifiedCount_++;
                if (identifiedCount_ >= 5) runAchMask_ |= ACH_SCHOLAR;
            }
            switch (sid) {
                case ScrollId::Identify:
                    awaitingIdentify_ = true;
                    pushMsg("Select an item to identify.", MsgColor::Highlight);
                    break;
                case ScrollId::EnchantWeapon:
                    if (player_.wieldedIdx >= 0) {
                        Item& w = inventory_[player_.wieldedIdx];
                        w.plus++;
                        w.dmgPlus++;
                        w.cursed = false;
                        w.identified = true;
                        pushMsg("Your weapon glows blue!", MsgColor::Good);
                    } else {
                        pushMsg("Your hands tingle.", MsgColor::Normal);
                    }
                    break;
                case ScrollId::EnchantArmor:
                    if (player_.armorIdx >= 0) {
                        Item& a = inventory_[player_.armorIdx];
                        a.plus++;
                        a.cursed = false;
                        a.identified = true;
                        pushMsg("Your armor glows silver!", MsgColor::Good);
                    } else {
                        pushMsg("Your skin tingles.", MsgColor::Normal);
                    }
                    break;
                case ScrollId::Teleport:
                    teleportPlayer();
                    pushMsg("Whoosh! Teleported!", MsgColor::Highlight);
                    break;
                case ScrollId::MagicMapping:
                    for (int y = 0; y < MAP_H; ++y)
                        for (int x = 0; x < MAP_W; ++x)
                            if (map_.tiles[y][x] != Tile::Void) map_.explored[y][x] = true;
                    pushMsg("Floor map revealed!", MsgColor::Highlight);
                    break;
                case ScrollId::RemoveCurse:
                    for (auto& inv : inventory_) inv.cursed = false;
                    pushMsg("You feel a curse lifted!", MsgColor::Good);
                    break;
                case ScrollId::HoldMonster:
                    for (auto& m : monsters_) {
                        if (m.alive && std::max(std::abs(m.x - player_.x), std::abs(m.y - player_.y)) <= 5) {
                            m.held = true;
                        }
                    }
                    pushMsg("Nearby monsters freeze!", MsgColor::Good);
                    break;
                case ScrollId::Aggravate:
                    for (auto& m : monsters_) {
                        if (m.alive) m.awake = true;
                    }
                    pushMsg("You hear a high-pitched noise!", MsgColor::Warn);
                    break;
                default:
                    break;
            }
            if (it.count > 1) it.count--;
            else {
                if (player_.wieldedIdx > idx) player_.wieldedIdx--;
                if (player_.armorIdx > idx) player_.armorIdx--;
                if (player_.ringIdx > idx) player_.ringIdx--;
                inventory_.erase(inventory_.begin() + idx);
            }
            endPlayerTurn(true);
            return true;
        }
        case ItemKind::Amulet:
            return false;
    }
    return false;
}

bool RogueGame::dropInventoryItem(int idx) {
    if (idx < 0 || idx >= static_cast<int>(inventory_.size())) return false;
    Item it = inventory_[idx];
    if (it.equipped && it.cursed) {
        pushMsg("It's cursed! Can't drop.", MsgColor::Warn);
        return false;
    }
    if (player_.wieldedIdx == idx) player_.wieldedIdx = -1;
    else if (player_.wieldedIdx > idx) player_.wieldedIdx--;
    if (player_.armorIdx == idx) player_.armorIdx = -1;
    else if (player_.armorIdx > idx) player_.armorIdx--;
    if (player_.ringIdx == idx) player_.ringIdx = -1;
    else if (player_.ringIdx > idx) player_.ringIdx--;

    it.equipped = false;
    it.x = player_.x;
    it.y = player_.y;
    groundItems_.push_back(it);
    pushMsg("Dropped " + getItemName(it), MsgColor::Normal);
    inventory_.erase(inventory_.begin() + idx);
    endPlayerTurn(true);
    return true;
}

RunResult RogueGame::buildRunResult() const {
    RunResult res;
    res.score = computeScore();
    res.floor = player_.maxFloorReached;
    res.level = player_.level;
    res.gold = player_.gold;
    res.kills = player_.kills;
    res.turns = player_.turns;
    res.escaped = (outcome_ == GameOutcome::Escaped);
    res.hasAmulet = player_.hasAmulet;
    std::snprintf(res.cause, sizeof(res.cause), "%s", deathCause_.c_str());
    return res;
}

} // namespace rogue
