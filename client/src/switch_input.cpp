#ifdef __SWITCH__
#include <switch.h>
#include <game.h>

void update_switch_input() {
    static PadState pad;
    static bool initialized = false;
    if (!initialized) {
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
        initialized = true;
    }
    padUpdate(&pad);
    const u64 held = padGetButtons(&pad);
    const u64 down = padGetButtonsDown(&pad);
    const HidAnalogStickState move = padGetStickPos(&pad, 0);
    const HidAnalogStickState look = padGetStickPos(&pad, 1);
    auto key = [](int code, bool pressed) {
        int& state = keys[code & ~0x40000000];
        if (pressed && (state == Input::NONE || state == Input::UP)) state = Input::DOWN;
        if (!pressed && (state == Input::PRESSED || state == Input::DOWN)) state = Input::UP;
    };
    key(SDLK_ESCAPE, held & HidNpadButton_Plus);
    if (Game::screen != Game::Screen::Playing || !player) {
        key(SDLK_UP, held & HidNpadButton_Up);
        key(SDLK_DOWN, held & HidNpadButton_Down);
        key(SDLK_RETURN, held & HidNpadButton_A);
        return;
    }
    key(SDLK_F5, held & HidNpadButton_StickR);
    key(SDLK_w, move.y > 8000);
    key(SDLK_s, move.y < -8000);
    key(SDLK_a, move.x < -8000);
    key(SDLK_d, move.x > 8000);
    key(SDLK_SPACE, held & HidNpadButton_A);
    key(SDLK_LCTRL, held & HidNpadButton_StickL || (!survival_mode && (held & HidNpadButton_B)));
    key(SDLK_LSHIFT, survival_mode && (held & HidNpadButton_B));
    key(SDLK_i, held & HidNpadButton_X);
    key(SDLK_TAB, held & HidNpadButton_Y);

    if (down & HidNpadButton_R) player->hotbar.set_index((player->hotbar.get_index() + 1) % 9);
    if (down & HidNpadButton_L) player->hotbar.set_index((player->hotbar.get_index() + 8) % 9);
    if (down & HidNpadButton_ZR) buttons[0] = Input::DOWN;
    if (down & HidNpadButton_ZL) buttons[2] = Input::DOWN;
    if (!show_inventory) {
        const float scale = 2.0f * (float)delta_time / 32767.0f;
        vec3 right = normalize(cross(player->cam.up, player->cam.forward));
        if (std::abs(look.x) > 5000)
            player->cam.forward = rotate(player->cam.forward, look.x * scale, player->cam.up);
        if (std::abs(look.y) > 5000) {
            vec3 forward = rotate(player->cam.forward, -look.y * scale, right);
            if (dot(right, cross(player->cam.up, forward)) > 0) player->cam.forward = forward;
        }
    }
}
#endif
