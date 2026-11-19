#pragma once

#include <cstdint>
#include <string>

namespace rogue {

constexpr int MAP_W = 60;
constexpr int MAP_H = 21;

constexpr int VIEW_W = 40;
constexpr int VIEW_H = 13;

constexpr int AMULET_FLOOR = 16;
constexpr int MAX_INVENTORY = 20;
constexpr int MAX_MONSTERS = 28;
constexpr int MAX_GROUND_ITEMS = 32;
constexpr int MAX_ROOMS = 9;

enum class Tile : uint8_t {
    Void = 0,
    WallH,
    WallV,
    Floor,
    Corridor,
    Door,
    StairsDown,
    StairsUp,
    Trap,
    SecretDoor,
};

enum class TrapKind : uint8_t {
    BearTrap = 0,
    DartTrap,
    TeleportTrap,
    SleepGas,
    Pit,
};

enum class ItemKind : uint8_t {
    Food = 0,
    Weapon,
    Armor,
    Potion,
    Scroll,
    Ring,
    Amulet,
};

enum class WeaponId : uint8_t {
    Dagger = 0,
    Mace,
    LongSword,
    BattleAxe,
    TwoHandSword,
    COUNT
};

enum class ArmorId : uint8_t {
    Leather = 0,
    RingMail,
    ScaleMail,
    ChainMail,
    SplintMail,
    PlateMail,
    COUNT
};

enum class PotionId : uint8_t {
    Healing = 0,
    ExtraHealing,
    GainStrength,
    RestoreStrength,
    Poison,
    Confusion,
    Blindness,
    Haste,
    SeeInvisible,
    COUNT
};

enum class ScrollId : uint8_t {
    Identify = 0,
    EnchantWeapon,
    EnchantArmor,
    Teleport,
    MagicMapping,
    RemoveCurse,
    HoldMonster,
    Aggravate,
    COUNT
};

enum class RingId : uint8_t {
    Protection = 0,
    AddStrength,
    SustainStrength,
    Regeneration,
    Searching,
    COUNT
};

struct Item {
    ItemKind kind = ItemKind::Food;
    uint8_t subId = 0;      // WeaponId / ArmorId / PotionId / ScrollId / RingId
    int8_t plus = 0;        // Enchantment (+hit/+dmg or +AC)
    int8_t dmgPlus = 0;     // Extra damage bonus for weapons
    uint8_t count = 1;
    bool cursed = false;
    bool equipped = false;
    bool identified = false;
    int16_t x = 0;
    int16_t y = 0;
};

enum class MonsterId : uint8_t {
    Bat = 0,
    Emu,
    Kobold,
    Rattlesnake,
    Hobgoblin,
    Orc,
    Zombie,
    IceMonster,
    Centaur,
    RustMonster,
    Wraith,
    Troll,
    Vampire,
    Medusa,
    Dragon,
    Jabberwock,
    COUNT
};

struct Monster {
    MonsterId id = MonsterId::Bat;
    int16_t x = 0;
    int16_t y = 0;
    int16_t hp = 1;
    int16_t maxHp = 1;
    int8_t ac = 9;
    bool awake = false;
    bool held = false;
    bool invisible = false;
    bool alive = false;
};

struct Room {
    int16_t x = 0;
    int16_t y = 0;
    int16_t w = 0;
    int16_t h = 0;
    bool dark = false;
    bool exists = false;
};

struct GoldPile {
    int16_t x = 0;
    int16_t y = 0;
    int16_t amount = 0;
    bool exists = false;
};

struct TrapCell {
    int16_t x = 0;
    int16_t y = 0;
    TrapKind kind = TrapKind::BearTrap;
    bool discovered = false;
    bool exists = false;
};

struct Player {
    int16_t x = 0;
    int16_t y = 0;
    int16_t hp = 12;
    int16_t maxHp = 12;
    int8_t str = 16;
    int8_t maxStr = 16;
    int8_t baseAc = 9; // Classic Rogue: lower is better (10=unarmored, leather=8, plate=3)
    int16_t level = 1;
    int32_t xp = 0;
    int32_t nextXp = 10;
    int32_t gold = 0;
    int16_t hunger = 1250; // Classic Rogue ~1300 turns
    int32_t turns = 0;
    int16_t floor = 1;
    int16_t maxFloorReached = 1;
    int16_t kills = 0;
    bool hasAmulet = false;

    // Status timers
    int16_t confusedTurns = 0;
    int16_t blindTurns = 0;
    int16_t hasteTurns = 0;
    int16_t frozenTurns = 0;
    int16_t bearTrapTurns = 0;
    bool seeInvisible = false;

    int8_t wieldedIdx = -1;
    int8_t armorIdx = -1;
    int8_t ringIdx = -1;
};

enum class MsgColor : uint8_t {
    Normal = 0,
    Good,      // Green/Cyan: healing, level up, record
    Warn,      // Yellow/Orange: hungry, trap, status
    Danger,    // Red: heavy damage, poison, death
    Highlight, // Gold: gold, amulet, milestone
};

enum class Action : uint8_t {
    None = 0,
    MoveN,
    MoveS,
    MoveW,
    MoveE,
    MoveNW,
    MoveNE,
    MoveSW,
    MoveSE,
    Rest,
    Search,
    Descend,
    Ascend,
    UseStairs, // Enter on stairs: automatically > or <
    OpenInventory,
    ToggleMinimap,
    ToggleHelp,
    OpenPause,
    // Quick commands
    Quaff,
    Read,
    Eat,
    Wield,
    Wear,
    Drop,
};

enum class GameOutcome : uint8_t {
    Playing = 0,
    Dead,
    Escaped,
};

} // namespace rogue
