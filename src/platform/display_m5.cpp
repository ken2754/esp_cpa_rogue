#include "display_m5.hpp"
#include <algorithm>
#include <cstdio>

namespace platform {

// RGB565 Palette
static constexpr uint16_t C_BG          = 0x0000; // Black
static constexpr uint16_t C_DIVIDER     = 0x2124; // Dark slate
static constexpr uint16_t C_WALL_LIT    = 0x7BEF; // Light gray-blue
static constexpr uint16_t C_WALL_FOG    = 0x2965; // Dim slate
static constexpr uint16_t C_FLOOR_LIT   = 0x52AA; // Medium gray
static constexpr uint16_t C_FLOOR_FOG   = 0x18E3; // Very dim gray
static constexpr uint16_t C_CORR_LIT    = 0x632C; // Corridor gray
static constexpr uint16_t C_CORR_FOG    = 0x2104; // Dim corridor
static constexpr uint16_t C_DOOR_LIT    = 0xD465; // Warm wood amber
static constexpr uint16_t C_DOOR_FOG    = 0x5201; // Dim wood
static constexpr uint16_t C_STAIRS      = 0x07FF; // Bright cyan
static constexpr uint16_t C_GOLD        = 0xFFE0; // Bright gold/yellow
static constexpr uint16_t C_TRAP        = 0xF800; // Bright red
static constexpr uint16_t C_PLAYER      = 0xFFFF; // Pure white
static constexpr uint16_t C_PLAYER_CONF = 0xF81F; // Magenta when confused
static constexpr uint16_t C_PLAYER_HAST = 0x07FF; // Cyan when hasted
static constexpr uint16_t C_TEXT        = 0xDEFB; // Soft white
static constexpr uint16_t C_DIM         = 0x7BEF; // Gray
static constexpr uint16_t C_GOOD        = 0x07E0; // Green
static constexpr uint16_t C_WARN        = 0xFD20; // Orange/Yellow
static constexpr uint16_t C_DANGER      = 0xF800; // Red
static constexpr uint16_t C_CYAN        = 0x07FF; // Cyan
static constexpr uint16_t C_MAGENTA     = 0xF81F; // Magenta

static void drawTileChar(LovyanGFX* gfx, int x, int y, char ch, uint16_t fg) {
    gfx->setTextColor(fg, C_BG);
    gfx->drawChar(static_cast<uint16_t>(static_cast<uint8_t>(ch)), x, y);
}

void DisplayM5::begin() {
    if (initialized_) return;
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(110);

    canvas_ = std::make_unique<M5Canvas>(&M5Cardputer.Display);
    canvas_->setColorDepth(16);
    if (canvas_->createSprite(240, 135) != nullptr) {
        useSprite_ = true;
    } else {
        canvas_->setColorDepth(8);
        if (canvas_->createSprite(240, 135) != nullptr) {
            useSprite_ = true;
        } else {
            canvas_.reset();
            useSprite_ = false;
        }
    }

    gfx_ = useSprite_ ? static_cast<LovyanGFX*>(canvas_.get())
                      : static_cast<LovyanGFX*>(&M5Cardputer.Display);
    gfx_->setFont(&fonts::Font0);
    gfx_->setTextSize(1);
    gfx_->setTextWrap(false);
    initialized_ = true;
}

void DisplayM5::clearScreen(uint16_t bg) {
    if (!gfx_) return;
    gfx_->fillScreen(bg);
}

void DisplayM5::present() {
    if (useSprite_ && canvas_) {
        canvas_->pushSprite(0, 0);
    }
}

uint16_t DisplayM5::msgColor565(rogue::MsgColor c) const {
    switch (c) {
        case rogue::MsgColor::Good:      return C_GOOD;
        case rogue::MsgColor::Warn:      return C_WARN;
        case rogue::MsgColor::Danger:    return C_DANGER;
        case rogue::MsgColor::Highlight: return C_GOLD;
        default:                         return C_TEXT;
    }
}

uint16_t DisplayM5::itemColor565(rogue::ItemKind k) const {
    switch (k) {
        case rogue::ItemKind::Food:   return 0xFD20; // Orange
        case rogue::ItemKind::Weapon: return 0xAD5F; // Steel blue
        case rogue::ItemKind::Armor:  return 0x4C9F; // Royal blue
        case rogue::ItemKind::Potion: return 0xF81F; // Magenta
        case rogue::ItemKind::Scroll: return 0x07FF; // Cyan
        case rogue::ItemKind::Ring:   return 0xB39F; // Violet
        case rogue::ItemKind::Amulet: return C_GOLD; // Gold
    }
    return C_TEXT;
}

uint16_t DisplayM5::monsterColor565(rogue::MonsterId id) const {
    const auto& def = rogue::getMonsterDef(id);
    switch (def.tier) {
        case 0: return 0x87E0; // Light green
        case 1: return 0xFD20; // Orange
        case 2: return 0xF81F; // Magenta
        case 3: return 0xF800; // Crimson red
    }
    return C_TEXT;
}

void DisplayM5::renderTitle(const rogue::RecordsData& rec) {
    clearScreen(C_BG);

    // Decorative border
    gfx_->drawRect(2, 2, 236, 131, C_DIVIDER);

    // Colorful Rogue Banner
    gfx_->setTextColor(C_GOLD, C_BG);
    gfx_->setCursor(24, 10);
    gfx_->print("====  R O G U E  A D V  ====");

    gfx_->setTextColor(C_CYAN, C_BG);
    gfx_->setCursor(42, 22);
    gfx_->print("Cardputer ADV Edition");

    // Hero vs Dragon motif
    gfx_->setTextColor(C_PLAYER, C_BG);
    gfx_->setCursor(84, 36);
    gfx_->print("@");
    gfx_->setTextColor(C_WALL_LIT, C_BG);
    gfx_->print(" .. ");
    gfx_->setTextColor(C_GOLD, C_BG);
    gfx_->print(",");
    gfx_->setTextColor(C_WALL_LIT, C_BG);
    gfx_->print(" .. ");
    gfx_->setTextColor(C_DANGER, C_BG);
    gfx_->print("D");

    // Personal Best Box
    gfx_->drawRect(10, 48, 220, 28, 0x3186);
    gfx_->setTextColor(C_TEXT, C_BG);
    char buf[44];
    if (rec.totalRuns > 0) {
        std::snprintf(buf, sizeof(buf), "Best: B%dF   Top Score: %ld",
                      rec.bestFloor, static_cast<long>(rec.bestScore));
        gfx_->setCursor(18, 53);
        gfx_->setTextColor(C_GOLD, C_BG);
        gfx_->print(buf);

        std::snprintf(buf, sizeof(buf), "Runs:%ld  Wins:%ld  Badges:%d/%d",
                      static_cast<long>(rec.totalRuns),
                      static_cast<long>(rec.totalWins),
                      rogue::countAchievements(rec.achievements),
                      rogue::NUM_ACHIEVEMENTS);
        gfx_->setCursor(18, 64);
        gfx_->setTextColor(C_TEXT, C_BG);
        gfx_->print(buf);
    } else {
        gfx_->setCursor(30, 58);
        gfx_->setTextColor(C_DIM, C_BG);
        gfx_->print("No runs yet. Claim the Amulet!");
    }

    // Menu
    gfx_->setCursor(38, 84);
    gfx_->setTextColor(C_GOOD, C_BG);
    gfx_->print("[Enter / N]  New Adventure");

    gfx_->setCursor(38, 97);
    gfx_->setTextColor(C_CYAN, C_BG);
    gfx_->print("[R]          Records & Badges");

    gfx_->setCursor(38, 110);
    gfx_->setTextColor(C_TEXT, C_BG);
    gfx_->print("[?]          Controls & Guide");

    present();
}

void DisplayM5::renderGame(const rogue::RogueGame& game, const rogue::RecordsData& rec) {
    clearScreen(C_BG);

    // 1. Top Message Bar (y=0..8)
    if (game.hasMessages()) {
        const auto& msg = game.frontMessage();
        gfx_->setTextColor(msgColor565(msg.color), C_BG);
        gfx_->setCursor(2, 1);
        std::string text = msg.text;
        if (text.size() > 39) text.resize(39);
        gfx_->print(text.c_str());
    } else {
        gfx_->setTextColor(C_DIM, C_BG);
        gfx_->setCursor(2, 1);
        if (game.player().hasAmulet) {
            gfx_->setTextColor(C_GOLD, C_BG);
            gfx_->print("Escape to B1F < stairs with Amulet!");
        } else {
            char hint[42];
            std::snprintf(hint, sizeof(hint), "Find stairs '>'  [i]Inv [m]Map [?]Help");
            gfx_->print(hint);
        }
    }

    gfx_->drawFastHLine(0, 9, 240, C_DIVIDER);

    // 2. Viewport (40x13 tiles, each 6x8px, y=10..113)
    const auto& map = game.map();
    const auto& pl = game.player();
    int cx = game.camX();
    int cy = game.camY();

    for (int row = 0; row < rogue::VIEW_H; ++row) {
        int my = cy + row;
        int py = 10 + row * 8;
        for (int col = 0; col < rogue::VIEW_W; ++col) {
            int mx = cx + col;
            if (!map.inBounds(mx, my)) continue;
            if (!map.explored[my][mx]) continue;

            bool vis = map.visible[my][mx];
            rogue::Tile t = map.tiles[my][mx];
            char ch = ' ';
            uint16_t color = C_FLOOR_FOG;

            switch (t) {
                case rogue::Tile::Void: continue;
                case rogue::Tile::WallH: ch = '-'; color = vis ? C_WALL_LIT : C_WALL_FOG; break;
                case rogue::Tile::WallV: ch = '|'; color = vis ? C_WALL_LIT : C_WALL_FOG; break;
                case rogue::Tile::SecretDoor:
                    ch = (mx > 0 && mx < rogue::MAP_W - 1 &&
                          (map.tiles[my][mx - 1] == rogue::Tile::WallH || map.tiles[my][mx + 1] == rogue::Tile::WallH))
                             ? '-' : '|';
                    color = vis ? C_WALL_LIT : C_WALL_FOG;
                    break;
                case rogue::Tile::Floor: ch = '.'; color = vis ? C_FLOOR_LIT : C_FLOOR_FOG; break;
                case rogue::Tile::Corridor: ch = '#'; color = vis ? C_CORR_LIT : C_CORR_FOG; break;
                case rogue::Tile::Door: ch = '+'; color = vis ? C_DOOR_LIT : C_DOOR_FOG; break;
                case rogue::Tile::StairsDown: ch = '>'; color = vis ? C_STAIRS : C_WALL_FOG; break;
                case rogue::Tile::StairsUp: ch = '<'; color = vis ? C_GOLD : C_WALL_FOG; break;
                case rogue::Tile::Trap: {
                    bool disc = false;
                    for (int ti = 0; ti < map.trapCount; ++ti) {
                        if (map.traps[ti].exists && map.traps[ti].x == mx && map.traps[ti].y == my) {
                            disc = map.traps[ti].discovered;
                            break;
                        }
                    }
                    if (disc) { ch = '^'; color = C_TRAP; }
                    else { ch = '.'; color = vis ? C_FLOOR_LIT : C_FLOOR_FOG; }
                    break;
                }
            }

            if (ch != ' ') {
                drawTileChar(gfx_, col * 6, py, ch, color);
            }
        }
    }

    // Gold piles (visible or explored)
    for (int i = 0; i < rogue::MAX_ROOMS; ++i) {
        const auto& g = map.gold[i];
        if (!g.exists) continue;
        if (!map.explored[g.y][g.x]) continue;
        int col = g.x - cx, row = g.y - cy;
        if (col >= 0 && col < rogue::VIEW_W && row >= 0 && row < rogue::VIEW_H) {
            uint16_t c = map.visible[g.y][g.x] ? C_GOLD : 0x6300;
            drawTileChar(gfx_, col * 6, 10 + row * 8, '*', c);
        }
    }

    // Ground items (visible or explored)
    for (const auto& it : game.groundItems()) {
        if (!map.explored[it.y][it.x]) continue;
        int col = it.x - cx, row = it.y - cy;
        if (col >= 0 && col < rogue::VIEW_W && row >= 0 && row < rogue::VIEW_H) {
            uint16_t c = map.visible[it.y][it.x] ? itemColor565(it.kind) : C_DIM;
            drawTileChar(gfx_, col * 6, 10 + row * 8, game.getItemSymbol(it.kind), c);
        }
    }

    // Monsters (only when visible!)
    for (const auto& m : game.monsters()) {
        if (!m.alive) continue;
        if (!map.visible[m.y][m.x]) continue;
        int col = m.x - cx, row = m.y - cy;
        if (col >= 0 && col < rogue::VIEW_W && row >= 0 && row < rogue::VIEW_H) {
            const auto& def = rogue::getMonsterDef(m.id);
            drawTileChar(gfx_, col * 6, 10 + row * 8, def.symbol, monsterColor565(m.id));
        }
    }

    // Player '@'
    {
        int col = pl.x - cx, row = pl.y - cy;
        uint16_t pc = C_PLAYER;
        if (pl.confusedTurns > 0) pc = C_PLAYER_CONF;
        else if (pl.hasteTurns > 0) pc = C_PLAYER_HAST;
        drawTileChar(gfx_, col * 6, 10 + row * 8, '@', pc);
    }

    // 3. Bottom Status Bar (y=115..134)
    gfx_->drawFastHLine(0, 115, 240, C_DIVIDER);

    // Line 1: Floor, Lv, HP, AC, Str
    gfx_->setCursor(2, 118);
    if (pl.hasAmulet) {
        gfx_->setTextColor(C_GOLD, C_BG);
        gfx_->printf("B%dF* ", pl.floor);
    } else if (rec.bestFloor > 0 && pl.floor > rec.bestFloor) {
        gfx_->setTextColor(C_GOLD, C_BG);
        gfx_->printf("B%dF! ", pl.floor);
    } else {
        gfx_->setTextColor(C_CYAN, C_BG);
        gfx_->printf("B%dF ", pl.floor);
    }

    gfx_->setTextColor(C_TEXT, C_BG);
    gfx_->printf("Lv:%d ", pl.level);

    uint16_t hpCol = C_GOOD;
    if (pl.hp * 4 <= pl.maxHp) hpCol = C_DANGER;
    else if (pl.hp * 2 <= pl.maxHp) hpCol = C_WARN;
    gfx_->setTextColor(hpCol, C_BG);
    gfx_->printf("HP:%d/%d ", pl.hp, pl.maxHp);

    gfx_->setTextColor(C_TEXT, C_BG);
    gfx_->printf("AC:%d Str:%d", game.effectiveAc(), game.effectiveStr());

    // Line 2: Gold, XP, Hunger, Status/Turns
    gfx_->setCursor(2, 127);
    gfx_->setTextColor(C_GOLD, C_BG);
    gfx_->printf("G:%ld ", static_cast<long>(pl.gold));

    gfx_->setTextColor(C_TEXT, C_BG);
    gfx_->printf("Xp:%ld/%ld ", static_cast<long>(pl.xp), static_cast<long>(pl.nextXp));

    if (pl.hunger <= 0) {
        gfx_->setTextColor(C_DANGER, C_BG);
        gfx_->print("FAINT ");
    } else if (pl.hunger <= 150) {
        gfx_->setTextColor(C_WARN, C_BG);
        gfx_->print("Weak ");
    } else if (pl.hunger <= 300) {
        gfx_->setTextColor(C_GOLD, C_BG);
        gfx_->print("Hungry ");
    }

    if (pl.confusedTurns > 0) {
        gfx_->setTextColor(C_MAGENTA, C_BG);
        gfx_->print("CONF ");
    } else if (pl.blindTurns > 0) {
        gfx_->setTextColor(C_DANGER, C_BG);
        gfx_->print("BLND ");
    } else if (pl.hasteTurns > 0) {
        gfx_->setTextColor(C_CYAN, C_BG);
        gfx_->print("HAST ");
    } else {
        gfx_->setTextColor(C_DIM, C_BG);
        gfx_->printf("T:%ld", static_cast<long>(pl.turns));
    }

    present();
}

void DisplayM5::renderInventory(const rogue::RogueGame& game, int selectedIdx) {
    clearScreen(C_BG);
    const auto& inv = game.inventory();

    // Header
    gfx_->fillRect(0, 0, 240, 12, 0x10A2);
    gfx_->setCursor(4, 2);
    if (game.awaitingIdentify()) {
        gfx_->setTextColor(C_GOLD, 0x10A2);
        gfx_->print("IDENTIFY WHICH ITEM?");
    } else {
        gfx_->setTextColor(C_CYAN, 0x10A2);
        gfx_->printf("INVENTORY (%d/%d)", static_cast<int>(inv.size()), rogue::MAX_INVENTORY);
    }

    if (inv.empty()) {
        gfx_->setCursor(60, 60);
        gfx_->setTextColor(C_DIM, C_BG);
        gfx_->print("Your pack is empty.");
    } else {
        constexpr int visibleRows = 9;
        int start = 0;
        if (selectedIdx >= visibleRows) {
            start = selectedIdx - visibleRows + 1;
        }
        for (int r = 0; r < visibleRows && (start + r) < static_cast<int>(inv.size()); ++r) {
            int idx = start + r;
            const auto& it = inv[idx];
            int y = 16 + r * 11;
            bool sel = (idx == selectedIdx);

            if (sel) {
                gfx_->fillRect(2, y - 1, 236, 10, 0x1925);
            }

            gfx_->setCursor(4, y);
            gfx_->setTextColor(sel ? C_GOLD : C_DIM, sel ? 0x1925 : C_BG);
            gfx_->print(sel ? ">" : " ");

            gfx_->setTextColor(itemColor565(it.kind), sel ? 0x1925 : C_BG);
            gfx_->printf("%c ", game.getItemSymbol(it.kind));

            std::string name = game.getItemName(it);
            if (name.size() > 26) name.resize(26);
            gfx_->setTextColor(sel ? C_PLAYER : C_TEXT, sel ? 0x1925 : C_BG);
            gfx_->print(name.c_str());

            if (it.equipped) {
                gfx_->setTextColor(C_GOOD, sel ? 0x1925 : C_BG);
                gfx_->print(" [E]");
            }
            if (it.cursed && it.identified) {
                gfx_->setTextColor(C_DANGER, sel ? 0x1925 : C_BG);
                gfx_->print(" {C}");
            }
        }
    }

    // Footer
    gfx_->drawFastHLine(0, 121, 240, C_DIVIDER);
    gfx_->setCursor(4, 125);
    gfx_->setTextColor(C_DIM, C_BG);
    if (game.awaitingIdentify()) {
        gfx_->print("Up/Dn:Select   Enter:Identify   Esc:Cancel");
    } else {
        gfx_->print("Up/Dn:Sel  Enter:Use  d:Drop  Esc:Back");
    }

    present();
}

void DisplayM5::renderMinimap(const rogue::RogueGame& game) {
    clearScreen(C_BG);
    const auto& map = game.map();
    const auto& pl = game.player();

    gfx_->setCursor(4, 2);
    gfx_->setTextColor(C_CYAN, C_BG);
    gfx_->printf("FLOOR B%dF MAP", pl.floor);
    gfx_->setTextColor(C_DIM, C_BG);
    gfx_->print(" (m/Esc:Close)");

    // 60x21 at 3x3 px = 180x63 centered at x=30, y=22
    constexpr int ox = 30;
    constexpr int oy = 22;
    gfx_->drawRect(ox - 2, oy - 2, rogue::MAP_W * 3 + 4, rogue::MAP_H * 3 + 4, C_DIVIDER);

    for (int y = 0; y < rogue::MAP_H; ++y) {
        for (int x = 0; x < rogue::MAP_W; ++x) {
            if (!map.explored[y][x]) continue;
            bool vis = map.visible[y][x];
            uint16_t col = C_BG;
            switch (map.tiles[y][x]) {
                case rogue::Tile::Void: continue;
                case rogue::Tile::WallH:
                case rogue::Tile::WallV:
                case rogue::Tile::SecretDoor:
                    col = vis ? C_WALL_LIT : C_WALL_FOG; break;
                case rogue::Tile::Floor:
                    col = vis ? 0x3186 : 0x1082; break;
                case rogue::Tile::Corridor:
                    col = vis ? C_CORR_LIT : C_CORR_FOG; break;
                case rogue::Tile::Door:
                    col = C_DOOR_LIT; break;
                case rogue::Tile::StairsDown:
                    col = C_STAIRS; break;
                case rogue::Tile::StairsUp:
                    col = C_GOLD; break;
                case rogue::Tile::Trap:
                    col = 0x3186; break;
            }
            gfx_->fillRect(ox + x * 3, oy + y * 3, 3, 3, col);
        }
    }

    // Ground items & gold on minimap
    for (int i = 0; i < rogue::MAX_ROOMS; ++i) {
        if (map.gold[i].exists && map.explored[map.gold[i].y][map.gold[i].x]) {
            gfx_->fillRect(ox + map.gold[i].x * 3, oy + map.gold[i].y * 3, 3, 3, C_GOLD);
        }
    }
    for (const auto& it : game.groundItems()) {
        if (map.explored[it.y][it.x]) {
            gfx_->fillRect(ox + it.x * 3, oy + it.y * 3, 3, 3, itemColor565(it.kind));
        }
    }

    // Visible monsters
    for (const auto& m : game.monsters()) {
        if (m.alive && map.visible[m.y][m.x]) {
            gfx_->fillRect(ox + m.x * 3, oy + m.y * 3, 3, 3, C_DANGER);
        }
    }

    // Player
    gfx_->fillRect(ox + pl.x * 3, oy + pl.y * 3, 3, 3, C_PLAYER);

    // Legend
    gfx_->setCursor(16, 102);
    gfx_->setTextColor(C_PLAYER, C_BG); gfx_->print("@:You ");
    gfx_->setTextColor(C_STAIRS, C_BG); gfx_->print(">:Stairs ");
    gfx_->setTextColor(C_GOLD, C_BG);   gfx_->print("*:Gold ");
    gfx_->setTextColor(C_DANGER, C_BG); gfx_->print("M:Foe");

    present();
}

void DisplayM5::renderRecords(const rogue::RecordsData& rec, int tab, bool confirmClear) {
    clearScreen(C_BG);

    // Tab bar
    gfx_->fillRect(0, 0, 240, 12, 0x10A2);
    gfx_->setCursor(4, 2);
    gfx_->setTextColor(tab == 0 ? C_GOLD : C_DIM, 0x10A2);
    gfx_->print("[1:HALL OF FAME] ");
    gfx_->setTextColor(tab == 1 ? C_GOLD : C_DIM, 0x10A2);
    gfx_->printf("[2:BADGES %d/%d]", rogue::countAchievements(rec.achievements), rogue::NUM_ACHIEVEMENTS);

    if (tab == 0) {
        gfx_->setCursor(4, 15);
        gfx_->setTextColor(C_CYAN, C_BG);
        gfx_->printf("Best:B%dF Top:%ld Wins:%ld/%ld",
                       rec.bestFloor, static_cast<long>(rec.bestScore),
                       static_cast<long>(rec.totalWins), static_cast<long>(rec.totalRuns));
        gfx_->drawFastHLine(0, 25, 240, C_DIVIDER);

        bool any = false;
        for (int i = 0; i < rogue::MAX_HALL_ENTRIES; ++i) {
            const auto& e = rec.hall[i];
            if (e.runNum == 0) continue;
            any = true;
            int y = 28 + i * 11;
            gfx_->setCursor(4, y);
            gfx_->setTextColor(i == 0 ? C_GOLD : C_TEXT, C_BG);
            gfx_->printf("#%d %5ld B%-2dF ", i + 1, static_cast<long>(e.score), e.floor);
            gfx_->setTextColor(e.escaped ? C_GOOD : C_DIM, C_BG);
            char cbuf[21];
            std::snprintf(cbuf, sizeof(cbuf), "%.20s", e.cause);
            gfx_->print(cbuf);
        }
        if (!any) {
            gfx_->setCursor(46, 64);
            gfx_->setTextColor(C_DIM, C_BG);
            gfx_->print("No completed runs yet.");
        }
    } else {
        const auto* list = rogue::getAchievementList();
        for (int i = 0; i < rogue::NUM_ACHIEVEMENTS; ++i) {
            int y = 16 + i * 10;
            bool unlocked = (rec.achievements & static_cast<uint32_t>(list[i].bit)) != 0;
            gfx_->setCursor(4, y);
            gfx_->setTextColor(unlocked ? C_GOLD : C_DIM, C_BG);
            gfx_->print(unlocked ? "[X] " : "[ ] ");
            gfx_->setTextColor(unlocked ? C_TEXT : 0x4208, C_BG);
            gfx_->printf("%-13s", list[i].name);
            gfx_->setTextColor(unlocked ? C_DIM : 0x3186, C_BG);
            char dbuf[20];
            std::snprintf(dbuf, sizeof(dbuf), "%.19s", list[i].desc);
            gfx_->print(dbuf);
        }
    }

    gfx_->drawFastHLine(0, 121, 240, C_DIVIDER);
    gfx_->setCursor(4, 125);
    if (confirmClear) {
        gfx_->setTextColor(C_DANGER, C_BG);
        gfx_->print("Reset ALL records? [Y]Yes  [N]No");
    } else {
        gfx_->setTextColor(C_DIM, C_BG);
        gfx_->print("L/R/Tab:Switch  Del:Reset  Esc:Back");
    }

    present();
}

void DisplayM5::renderHelp() {
    clearScreen(C_BG);
    gfx_->fillRect(0, 0, 240, 12, 0x10A2);
    gfx_->setCursor(4, 2);
    gfx_->setTextColor(C_CYAN, 0x10A2);
    gfx_->print("CONTROLS & ROGUE GUIDE");

    const char* lines[] = {
        "Move:  h/j/k/l or Fn+Arrows (; . , /)",
        "Diag:  y / u / b / n",
        "Stairs: > / < or Enter on stairs",
        "Rest:  . (dot)      Search: s",
        "Pack:  i (Use/Equip/Drop items)",
        "Quick: q:Quaff r:Read e:Eat w:Wield",
        "Map:   m (Minimap)  Esc(`):Pause",
        "Goal:  Reach B16F, grab Amulet (,),",
        "       then ascend to B1F < to win!"
    };
    for (int i = 0; i < 9; ++i) {
        gfx_->setCursor(4, 16 + i * 11);
        gfx_->setTextColor(i >= 7 ? C_GOLD : C_TEXT, C_BG);
        gfx_->print(lines[i]);
    }

    gfx_->drawFastHLine(0, 121, 240, C_DIVIDER);
    gfx_->setCursor(4, 125);
    gfx_->setTextColor(C_DIM, C_BG);
    gfx_->print("Press any key to return");
    present();
}

void DisplayM5::renderPause(const rogue::RogueGame& game) {
    clearScreen(C_BG);
    gfx_->drawRect(24, 14, 192, 106, C_DIVIDER);
    gfx_->setCursor(86, 22);
    gfx_->setTextColor(C_GOLD, C_BG);
    gfx_->print("PAUSED");

    gfx_->setCursor(44, 38);
    gfx_->setTextColor(C_TEXT, C_BG);
    gfx_->printf("Floor: B%dF   Score: %d", game.player().floor, game.computeScore());

    gfx_->setCursor(52, 56);
    gfx_->setTextColor(C_GOOD, C_BG);
    gfx_->print("[Esc / Enter] Resume");

    gfx_->setCursor(52, 70);
    gfx_->setTextColor(C_CYAN, C_BG);
    gfx_->print("[R] Records & Badges");

    gfx_->setCursor(52, 84);
    gfx_->setTextColor(C_TEXT, C_BG);
    gfx_->print("[?] Controls");

    gfx_->setCursor(52, 98);
    gfx_->setTextColor(C_WARN, C_BG);
    gfx_->print("[Q] Quit to Title");

    present();
}

void DisplayM5::renderGameOver(const rogue::RunResult& res) {
    clearScreen(C_BG);
    gfx_->drawRect(2, 2, 236, 131, res.escaped ? C_GOLD : C_DANGER);

    gfx_->setCursor(res.escaped ? 42 : 72, 10);
    gfx_->setTextColor(res.escaped ? C_GOLD : C_DANGER, C_BG);
    gfx_->print(res.escaped ? "VICTORY! YOU ESCAPED!" : "REST IN PEACE");

    gfx_->setCursor(16, 25);
    gfx_->setTextColor(C_TEXT, C_BG);
    gfx_->print(res.cause);

    gfx_->drawFastHLine(10, 36, 220, C_DIVIDER);

    gfx_->setCursor(16, 42);
    gfx_->setTextColor(C_GOLD, C_BG);
    gfx_->printf("Score: %ld", static_cast<long>(res.score));
    if (res.newBestScore) {
        gfx_->setTextColor(C_GOOD, C_BG);
        gfx_->print(" *NEW BEST!*");
    }

    gfx_->setCursor(16, 54);
    gfx_->setTextColor(C_CYAN, C_BG);
    gfx_->printf("Floor: B%dF", res.floor);
    if (res.newBestFloor) {
        gfx_->setTextColor(C_GOOD, C_BG);
        gfx_->print(" *DEEPEST!*");
    }

    gfx_->setCursor(16, 66);
    gfx_->setTextColor(C_TEXT, C_BG);
    gfx_->printf("Lv:%d  Gold:%ld  Kills:%d  Turns:%ld",
                   res.level, static_cast<long>(res.gold), res.kills, static_cast<long>(res.turns));

    int yNote = 80;
    if (res.hallRank >= 0) {
        gfx_->setCursor(16, yNote);
        gfx_->setTextColor(C_GOLD, C_BG);
        gfx_->printf("Hall of Fame Rank #%d!", res.hallRank + 1);
        yNote += 12;
    }
    if (res.newlyUnlockedAch != 0) {
        gfx_->setCursor(16, yNote);
        gfx_->setTextColor(C_GOOD, C_BG);
        gfx_->printf("Unlocked %d new badge(s)!", rogue::countAchievements(res.newlyUnlockedAch));
    }

    gfx_->drawFastHLine(10, 117, 220, C_DIVIDER);
    gfx_->setCursor(14, 122);
    gfx_->setTextColor(C_DIM, C_BG);
    gfx_->print("[Enter] Play Again  [R] Records  [Esc] Title");

    present();
}

} // namespace platform
