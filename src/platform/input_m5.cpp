#include "input_m5.hpp"
#include <M5Cardputer.h>

namespace platform {

static KeyEvent decodeKeysState(const Keyboard_Class::KeysState& st) {
    KeyEvent ev;
    ev.fn = st.fn;
    ev.shift = st.shift;
    ev.ctrl = st.ctrl;
    ev.enter = st.enter;
    ev.del = st.del;
    ev.space = st.space;
    ev.tab = st.tab;

    if (!st.word.empty()) {
        char c = st.word[0];
        if (st.fn) {
            if (c == ';') { ev.up = true; ev.valid = true; return ev; }
            if (c == '.') { ev.down = true; ev.valid = true; return ev; }
            if (c == ',') { ev.left = true; ev.valid = true; return ev; }
            if (c == '/') { ev.right = true; ev.valid = true; return ev; }
            if (c == '`') { ev.esc = true; ev.valid = true; return ev; }
        }
        if (c == '`' || c == '~') {
            ev.esc = true;
            ev.valid = true;
            return ev;
        }
        ev.ch = c;
        ev.valid = true;
        return ev;
    }

    if (st.enter || st.del || st.space || st.tab) {
        ev.valid = true;
    }
    return ev;
}

KeyEvent InputHandler::poll() {
    uint32_t now = millis();
    bool changed = M5Cardputer.Keyboard.isChange();
    bool pressed = M5Cardputer.Keyboard.isPressed();

    if (!pressed) {
        wasPressed_ = false;
        lastEvent_ = KeyEvent{};
        return KeyEvent{};
    }

    Keyboard_Class::KeysState st = M5Cardputer.Keyboard.keysState();
    KeyEvent ev = decodeKeysState(st);
    if (!ev.valid) {
        return KeyEvent{};
    }

    if (changed || !wasPressed_) {
        wasPressed_ = true;
        holdStartMs_ = now;
        lastRepeatMs_ = now;
        lastEvent_ = ev;
        return ev;
    }

    // Key repeat for movement/navigation keys when held
    bool repeatable = ev.up || ev.down || ev.left || ev.right ||
                      ev.ch == 'h' || ev.ch == 'j' || ev.ch == 'k' || ev.ch == 'l' ||
                      ev.ch == 'y' || ev.ch == 'u' || ev.ch == 'b' || ev.ch == 'n' ||
                      ev.ch == '.';
    if (repeatable && (now - holdStartMs_ >= 260) && (now - lastRepeatMs_ >= 95)) {
        lastRepeatMs_ = now;
        return ev;
    }

    return KeyEvent{};
}

} // namespace platform
