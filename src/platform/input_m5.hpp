#pragma once

#include <cstdint>

namespace platform {

struct KeyEvent {
    bool valid = false;
    char ch = 0;
    bool enter = false;
    bool esc = false;
    bool del = false;
    bool space = false;
    bool tab = false;
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool fn = false;
    bool shift = false;
    bool ctrl = false;
};

class InputHandler {
public:
    KeyEvent poll();

private:
    uint32_t holdStartMs_ = 0;
    uint32_t lastRepeatMs_ = 0;
    bool wasPressed_ = false;
    KeyEvent lastEvent_{};
};

} // namespace platform
