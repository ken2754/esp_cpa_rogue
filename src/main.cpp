#include <M5Cardputer.h>
#include <esp_random.h>

#include "game/records.hpp"
#include "game/rogue.hpp"
#include "platform/display_m5.hpp"
#include "platform/input_m5.hpp"
#include "platform/storage_nvs.hpp"

enum class ScreenState {
    Title = 0,
    Playing,
    Inventory,
    Minimap,
    Records,
    Help,
    Pause,
    GameOver,
};

static rogue::RogueGame g_game;
static rogue::RecordsData g_records;
static rogue::RunResult g_lastResult;
static platform::DisplayM5 g_display;
static platform::InputHandler g_input;

static ScreenState g_state = ScreenState::Title;
static ScreenState g_returnState = ScreenState::Title;
static int g_invSelected = 0;
static int g_recTab = 0;
static bool g_confirmClear = false;
static bool g_dirty = true;

static void startNewAdventure() {
    uint32_t seed = esp_random() ^ millis();
    g_game.startNewGame(seed, g_records);
    g_invSelected = 0;
    g_state = ScreenState::Playing;
    g_dirty = true;
}

static void finishGameIfEnded() {
    if (g_game.outcome() != rogue::GameOutcome::Playing) {
        g_lastResult = g_game.buildRunResult();
        rogue::recordRun(g_records, g_lastResult, g_game.runAchievements());
        platform::saveRecords(g_records);
        g_state = ScreenState::GameOver;
        g_dirty = true;
    }
}

static void openInventoryWithFilter(rogue::ItemKind preferredKind, bool useFilter) {
    g_invSelected = 0;
    if (useFilter) {
        const auto& inv = g_game.inventory();
        for (size_t i = 0; i < inv.size(); ++i) {
            if (inv[i].kind == preferredKind) {
                g_invSelected = static_cast<int>(i);
                break;
            }
        }
    }
    g_state = ScreenState::Inventory;
    g_dirty = true;
}

static void handleTitleInput(const platform::KeyEvent& ev) {
    if (ev.enter || ev.ch == 'n' || ev.ch == 'N') {
        startNewAdventure();
    } else if (ev.ch == 'r' || ev.ch == 'R') {
        g_recTab = 0;
        g_confirmClear = false;
        g_returnState = ScreenState::Title;
        g_state = ScreenState::Records;
        g_dirty = true;
    } else if (ev.ch == '?' || ev.ch == '/') {
        g_returnState = ScreenState::Title;
        g_state = ScreenState::Help;
        g_dirty = true;
    }
}

static void handlePlayingInput(const platform::KeyEvent& ev) {
    // If multiple messages are queued, advance one message per keypress
    if (g_game.hasMessages()) {
        g_game.popMessage();
        if (g_game.hasMessages()) {
            g_dirty = true;
            return;
        }
    }

    rogue::Action act = rogue::Action::None;

    if (ev.up || ev.ch == 'k') act = rogue::Action::MoveN;
    else if (ev.down || ev.ch == 'j') act = rogue::Action::MoveS;
    else if (ev.left || ev.ch == 'h') act = rogue::Action::MoveW;
    else if (ev.right || ev.ch == 'l') act = rogue::Action::MoveE;
    else if (ev.ch == 'y') act = rogue::Action::MoveNW;
    else if (ev.ch == 'u') act = rogue::Action::MoveNE;
    else if (ev.ch == 'b') act = rogue::Action::MoveSW;
    else if (ev.ch == 'n') act = rogue::Action::MoveSE;
    else if (ev.ch == '.') act = rogue::Action::Rest;
    else if (ev.ch == 's') act = rogue::Action::Search;
    else if (ev.ch == '>') act = rogue::Action::Descend;
    else if (ev.ch == '<') act = rogue::Action::Ascend;
    else if (ev.enter) act = rogue::Action::UseStairs;
    else if (ev.ch == 'i') { openInventoryWithFilter(rogue::ItemKind::Food, false); return; }
    else if (ev.ch == 'e') { openInventoryWithFilter(rogue::ItemKind::Food, true); return; }
    else if (ev.ch == 'q') { openInventoryWithFilter(rogue::ItemKind::Potion, true); return; }
    else if (ev.ch == 'r') { openInventoryWithFilter(rogue::ItemKind::Scroll, true); return; }
    else if (ev.ch == 'w' || ev.ch == 'W') { openInventoryWithFilter(rogue::ItemKind::Weapon, true); return; }
    else if (ev.ch == 'm' || ev.ch == 'M') {
        g_state = ScreenState::Minimap;
        g_dirty = true;
        return;
    } else if (ev.ch == '?') {
        g_returnState = ScreenState::Playing;
        g_state = ScreenState::Help;
        g_dirty = true;
        return;
    } else if (ev.esc) {
        g_state = ScreenState::Pause;
        g_dirty = true;
        return;
    }

    if (act != rogue::Action::None) {
        g_game.step(act);
        if (g_game.awaitingIdentify()) {
            g_invSelected = 0;
            g_state = ScreenState::Inventory;
        }
        finishGameIfEnded();
        g_dirty = true;
    }
}

static void handleInventoryInput(const platform::KeyEvent& ev) {
    int count = static_cast<int>(g_game.inventory().size());
    if (ev.esc || ev.ch == 'i') {
        g_game.cancelIdentify();
        g_state = ScreenState::Playing;
        g_dirty = true;
        return;
    }
    if (count == 0) return;

    if (ev.up || ev.ch == 'k') {
        g_invSelected = (g_invSelected - 1 + count) % count;
        g_dirty = true;
    } else if (ev.down || ev.ch == 'j') {
        g_invSelected = (g_invSelected + 1) % count;
        g_dirty = true;
    } else if (ev.enter || ev.space) {
        if (g_game.awaitingIdentify()) {
            g_game.identifyInventoryItem(g_invSelected);
            g_state = ScreenState::Playing;
        } else {
            g_game.useInventoryItem(g_invSelected);
            if (!g_game.awaitingIdentify()) {
                g_state = ScreenState::Playing;
                finishGameIfEnded();
            } else {
                if (g_invSelected >= static_cast<int>(g_game.inventory().size())) {
                    g_invSelected = std::max(0, static_cast<int>(g_game.inventory().size()) - 1);
                }
            }
        }
        g_dirty = true;
    } else if (ev.ch == 'd' && !g_game.awaitingIdentify()) {
        g_game.dropInventoryItem(g_invSelected);
        if (g_invSelected >= static_cast<int>(g_game.inventory().size())) {
            g_invSelected = std::max(0, static_cast<int>(g_game.inventory().size()) - 1);
        }
        g_state = ScreenState::Playing;
        finishGameIfEnded();
        g_dirty = true;
    }
}

static void handleMinimapInput(const platform::KeyEvent& ev) {
    if (ev.esc || ev.ch == 'm' || ev.ch == 'M' || ev.enter || ev.space) {
        g_state = ScreenState::Playing;
        g_dirty = true;
    }
}

static void handleRecordsInput(const platform::KeyEvent& ev) {
    if (g_confirmClear) {
        if (ev.ch == 'y' || ev.ch == 'Y') {
            platform::clearRecords(g_records);
            g_confirmClear = false;
            g_dirty = true;
        } else if (ev.ch == 'n' || ev.ch == 'N' || ev.esc) {
            g_confirmClear = false;
            g_dirty = true;
        }
        return;
    }

    if (ev.esc) {
        g_state = g_returnState;
        g_dirty = true;
    } else if (ev.left || ev.right || ev.tab || ev.ch == '1' || ev.ch == '2') {
        if (ev.ch == '1') g_recTab = 0;
        else if (ev.ch == '2') g_recTab = 1;
        else g_recTab = 1 - g_recTab;
        g_dirty = true;
    } else if (ev.del) {
        g_confirmClear = true;
        g_dirty = true;
    }
}

static void handlePauseInput(const platform::KeyEvent& ev) {
    if (ev.esc || ev.enter) {
        g_state = ScreenState::Playing;
        g_dirty = true;
    } else if (ev.ch == 'r' || ev.ch == 'R') {
        g_recTab = 0;
        g_confirmClear = false;
        g_returnState = ScreenState::Pause;
        g_state = ScreenState::Records;
        g_dirty = true;
    } else if (ev.ch == '?') {
        g_returnState = ScreenState::Pause;
        g_state = ScreenState::Help;
        g_dirty = true;
    } else if (ev.ch == 'q' || ev.ch == 'Q') {
        g_state = ScreenState::Title;
        g_dirty = true;
    }
}

static void handleGameOverInput(const platform::KeyEvent& ev) {
    if (ev.enter || ev.ch == 'n' || ev.ch == 'N') {
        startNewAdventure();
    } else if (ev.ch == 'r' || ev.ch == 'R') {
        g_recTab = 0;
        g_confirmClear = false;
        g_returnState = ScreenState::GameOver;
        g_state = ScreenState::Records;
        g_dirty = true;
    } else if (ev.esc) {
        g_state = ScreenState::Title;
        g_dirty = true;
    }
}

void setup() {
    delay(50);
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true); // true = enable keyboard (works on Cardputer & Cardputer ADV)
    delay(50);

    g_display.begin();
    platform::loadRecords(g_records);
    g_state = ScreenState::Title;
    g_dirty = true;
}

void loop() {
    M5Cardputer.update();

    platform::KeyEvent ev = g_input.poll();
    if (ev.valid) {
        switch (g_state) {
            case ScreenState::Title:     handleTitleInput(ev); break;
            case ScreenState::Playing:   handlePlayingInput(ev); break;
            case ScreenState::Inventory: handleInventoryInput(ev); break;
            case ScreenState::Minimap:   handleMinimapInput(ev); break;
            case ScreenState::Records:   handleRecordsInput(ev); break;
            case ScreenState::Help:      g_state = g_returnState; g_dirty = true; break;
            case ScreenState::Pause:     handlePauseInput(ev); break;
            case ScreenState::GameOver:  handleGameOverInput(ev); break;
        }
    }

    if (g_dirty) {
        g_dirty = false;
        switch (g_state) {
            case ScreenState::Title:     g_display.renderTitle(g_records); break;
            case ScreenState::Playing:   g_display.renderGame(g_game, g_records); break;
            case ScreenState::Inventory: g_display.renderInventory(g_game, g_invSelected); break;
            case ScreenState::Minimap:   g_display.renderMinimap(g_game); break;
            case ScreenState::Records:   g_display.renderRecords(g_records, g_recTab, g_confirmClear); break;
            case ScreenState::Help:      g_display.renderHelp(); break;
            case ScreenState::Pause:     g_display.renderPause(g_game); break;
            case ScreenState::GameOver:  g_display.renderGameOver(g_lastResult); break;
        }
    }

    delay(10);
}
