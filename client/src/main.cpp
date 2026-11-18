#include <global.h>
#include <game.h>
#ifdef __SWITCH__
#include <switch.h>
#include <unistd.h>
void update_switch_input();
#endif

void handle_events()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            closed = true;
            break;
        }
        else if (event.type == SDL_KEYDOWN)
        {
            int index = event.key.keysym.sym & (~0x40000000);

            if (index < KEY_MAX && keys[index] == Input::NONE)
                keys[index] = Input::DOWN;
        }
        else if (event.type == SDL_KEYUP)
        {
            int index = event.key.keysym.sym & (~0x40000000);

            if (index < KEY_MAX)
                keys[index] = Input::UP;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            int index = event.button.button - 1;

            if (index < BUTTON_MAX && buttons[index] == Input::NONE)
                buttons[index] = Input::DOWN;
        }
        else if (event.type == SDL_MOUSEBUTTONUP)
        {
            int index = event.button.button - 1;

            if (index < BUTTON_MAX)
                buttons[index] = Input::UP;
        }
    }
}

int main(int argc, char** argv)
{
    const bool smoke_test = argc == 2 && std::string(argv[1]) == "--smoke-test";
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return 1;
    }
#ifdef __SWITCH__
    if (R_FAILED(romfsInit())) return 1;
    chdir("romfs:/");
    win_size = uvec2(1280, 720);
#else
    char* base = SDL_GetBasePath();
    if (base) { std::filesystem::current_path(base); SDL_free(base); }
#endif

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    win = SDL_CreateWindow
    (
        win_name,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        win_size.x,
        win_size.y,
        SDL_WINDOW_OPENGL | (is_fullscreen ? SDL_WINDOW_FULLSCREEN : 0)
    );

    SDL_GLContext context = SDL_GL_CreateContext(win);
    if (!win || !context || !gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("OpenGL 3.3 initialization failed: %s", SDL_GetError());
        return 2;
    }
    SDL_GL_SetSwapInterval(1);
    glViewport(0, 0, win_size.x, win_size.y);

    Game::init();
    if (smoke_test) {
        mouse_control = false;
        show_debug = false;
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
    int frame = 0;
    Chunk* edited_chunk = nullptr;
    ivec3 edited_pos(0);
    bool edit_placed = false;
    auto save_frame = [](const char* path) {
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, win_size.x, win_size.y, 32, SDL_PIXELFORMAT_RGBA32);
        glReadPixels(0, 0, win_size.x, win_size.y, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        // OpenGL reads from the bottom left; flip for a normal image.
        auto* pixels = static_cast<unsigned char*>(surface->pixels);
        for (int y = 0; y < surface->h / 2; ++y)
            for (int x = 0; x < surface->pitch; ++x)
                std::swap(pixels[y * surface->pitch + x], pixels[(surface->h - 1 - y) * surface->pitch + x]);
        SDL_SaveBMP(surface, path);
        SDL_FreeSurface(surface);
    };

    Uint64 this_frame = SDL_GetPerformanceCounter();
    Uint64 last_frame = 0;

    while (!closed)
    {
#ifdef __SWITCH__
        if (!appletMainLoop()) break;
#endif
        last_frame = this_frame;
        this_frame = SDL_GetPerformanceCounter();

        delta_time = std::min(0.05, (this_frame - last_frame) / (double)SDL_GetPerformanceFrequency());

        update_inputs();
        handle_events();
#ifdef __SWITCH__
        update_switch_input();
#endif

        Game::update();
        if (smoke_test) {
            ++frame;
            if (frame == 2) {
                player->pos.y += 5;
                player->cam.forward = normalize(vec3(0, -0.65f, 1));
            }
            if (frame == 90) {
                edited_chunk = world->get_chunk_from_pos(player->pos);
                for (int y = CHUNK_HEIGHT - 3; y >= 0; --y)
                    if (edited_chunk->get(ivec3(8, y, 12))->id != AIR) {
                        edited_pos = ivec3(8, y + 1, 12);
                        break;
                    }
                BlockState stone = { STONE, 0, 0 };
                edit_placed = world->place_block(edited_chunk, edited_pos, &stone);
            }
            if (frame == 91 && edit_placed) world->remove_block(edited_chunk, edited_pos);
        }
        Game::draw();
        if (smoke_test) {
            if (glGetError() != GL_NO_ERROR) { SDL_Log("Smoke test: OpenGL error"); return 3; }
            if (frame == 89) save_frame("smoke-before.bmp");
            if (frame == 90) save_frame("smoke-placed.bmp");
            if (frame == 91) save_frame("smoke-after.bmp");
            if (frame == 92) {
                if (!world->spawned || !edit_placed || edited_chunk->get(edited_pos)->id != AIR) return 4;
                SDL_Log("Smoke test passed: surface spawn, block placement/removal, no GL errors");
                closed = true;
            }
        }

        SDL_GL_SwapWindow(win);
    }

    Game::destroy();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(win);
    SDL_Quit();
#ifdef __SWITCH__
    romfsExit();
#endif

    return 0;
}
