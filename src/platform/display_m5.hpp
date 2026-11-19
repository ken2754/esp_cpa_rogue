#pragma once

#include "../game/records.hpp"
#include "../game/rogue.hpp"
#include <M5Cardputer.h>
#include <memory>

namespace platform {

class DisplayM5 {
public:
    void begin();

    void renderTitle(const rogue::RecordsData& rec);
    void renderGame(const rogue::RogueGame& game, const rogue::RecordsData& rec);
    void renderInventory(const rogue::RogueGame& game, int selectedIdx);
    void renderMinimap(const rogue::RogueGame& game);
    void renderRecords(const rogue::RecordsData& rec, int tab, bool confirmClear);
    void renderHelp();
    void renderPause(const rogue::RogueGame& game);
    void renderGameOver(const rogue::RunResult& res);

private:
    std::unique_ptr<M5Canvas> canvas_;
    LovyanGFX* gfx_ = nullptr;
    bool useSprite_ = false;
    bool initialized_ = false;

    void clearScreen(uint16_t bg = 0x0000);
    void present();
    uint16_t msgColor565(rogue::MsgColor c) const;
    uint16_t itemColor565(rogue::ItemKind k) const;
    uint16_t monsterColor565(rogue::MonsterId id) const;
};

} // namespace platform
