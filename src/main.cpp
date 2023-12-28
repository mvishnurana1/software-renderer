#include <cassert>
#include <chrono>
#include <cstdio>
#include <vector>

#define SDL_MAIN_HANDLED
#include "./include/SDL/SDL.h"

/*
    Unity build
*/
#include "platform_specific.cpp"
#include "maths.cpp"
#include "image.cpp"
#include "file.cpp"
#include "render.cpp"
#include "shaders.cpp"

/*
    App Shaders
*/
static blinn_shader_normal_map blinn_shader_normal_map;
static const int shader_count = 1;
static shader * shaders[shader_count] = {
    &blinn_shader_normal_map
};

/*
    Model Buffer
*/
static int model_count;
static model * models;

struct ui_state
{
    bool mouse_down{};
    bool mouse_down_this_frame{};
    bool mouse_in_window{};
    int mouse_x{};
    int mouse_y{};
};

struct application_state
{
    render_state gl_state;
    ui_state ui_state;

    model* active_model{};
    int active_model_idx{};

    shader* active_shader{};
    int active_shader_idx{};

    bool running{};
    bool use_fx = false;

    //active model rotation/transform
    v3 target_rot{};
    v3 target_trans{};

    //framebuffer clear color & lerp data
    hsla background_color{};
    float background_lerp_percent{};

    //true if the app will sleep in one frame.
    bool impending_sleep{};
};

SDL_Window* global_window = nullptr;
SDL_Surface* global_screen_surface = nullptr;
application_state global_app_state;

static int main_loop(application_state & app_state, SDL_Window* window, SDL_Surface* screen_surface);

//emscripten main loop
#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>

EM_BOOL one_iter(double time, void* userData) {
    main_loop(global_app_state, global_window, global_screen_surface);

    // Return true to keep the loop running.
    return EM_TRUE;
}
#endif

int main(int argc, char* args[]) {
    /* Load the models */
    load_models("./obj/conf.bin", models, model_count);

    const auto render_width = 256;
    const auto render_height = 256;

    /* Initialise application settings */
    global_app_state = application_state{
        //renderer state
        {
            v3{ 0, 0, 3 },
            v3{ 0, 0, 0 },
            v3{ 0, 1, 0 },
            v3{ 1, 1, 1 }.normalise(),
            identity(),
            projection(v3{ 0, 0, 3 }, v3{ 0, 0, 0 }),
            view_port(
                static_cast<float>(render_width) / 10,
                static_cast<float>(render_height) / 10,
                static_cast<float>(render_width) * 4 / 5,
                static_cast<float>(render_height) * 4 / 5
            )
        },
        //ui state
        { },
        //active model
        &models[2], 2,
        //active shader
        shaders[0], 0,
    };

    /* Initialise output buffers. The width/height values specified here determine rendering resolution */
    init_output_buffers(global_app_state.gl_state.output_buffers, render_width, render_height);
    printf("Rendering with Width:%d and Height:%d\n", render_width, render_height);

    global_app_state.background_color = rgb_to_hsl(eggshell);

    /* Setup initial model position and app background color */
    clear_output_buffers(global_app_state.gl_state.output_buffers, hsl_to_rgb(global_app_state.background_color));


    /* Initialise SDL and begin main loop */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Could not initialize SDL: %s.\n", SDL_GetError());
        exit(-1);
    }
    else
    {
        global_window = SDL_CreateWindow(
                            "Software Renderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            render_width, render_height, 0
                        );
        if (global_window == nullptr)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        }
        else
        {
            global_screen_surface = SDL_GetWindowSurface(global_window);

#ifdef EMSCRIPTEN

            //emscripten main loop
            emscripten_request_animation_frame_loop(one_iter, 0);

#else

            //desktop main loop
            global_app_state.running = true;
            while (global_app_state.running) {
                main_loop(global_app_state, global_window, global_screen_surface);
            }

            SDL_Quit();

#endif
        }
    }

    return 0;
}

static void poll_events(application_state& app_state);
static void draw_scene(application_state & app_state, SDL_Surface* screen_surface);
static void copy_frame_buffer_to_screen(application_state& app_state, SDL_Surface* screen_surface);

static int main_loop(application_state & app_state, SDL_Window* window, SDL_Surface* screen_surface)
{
    const auto start = std::chrono::high_resolution_clock::now();

    //get user input
    poll_events(app_state);

    //render the scene
    draw_scene(app_state, screen_surface);

    //blit the render to the window
    copy_frame_buffer_to_screen(app_state, screen_surface);

    //Update the window
    SDL_UpdateWindowSurface( window );

    //clear the output buffers for next frame
    clear_output_buffers(app_state.gl_state.output_buffers, hsl_to_rgb(app_state.background_color));

    //calculate and store delta time
    const auto stop = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    const auto frame_duration_seconds = duration.count() / 1000.0f;

    app_state.gl_state.dt = frame_duration_seconds;
    app_state.gl_state.culm_dt += frame_duration_seconds;

    return 0;
}

static void poll_events(application_state & app_state)
{
    SDL_GetMouseState(&app_state.ui_state.mouse_x, &app_state.ui_state.mouse_y);
    //invert mouse y - want bottom left to be window origin (matches output buffers)
    app_state.ui_state.mouse_y = app_state.gl_state.output_buffers.frame_buffer.height - app_state.ui_state.mouse_y;

    //reset mouse down state for this frame
    app_state.ui_state.mouse_down_this_frame = false;

    SDL_Event e;
    while (SDL_PollEvent(&e) != 0)
    {
        switch (e.type)
        {
        case SDL_QUIT: {
            app_state.running = false;
        }
        break;

        case SDL_MOUSEMOTION: {
            if (app_state.ui_state.mouse_down) {
                app_state.target_rot.x += e.motion.yrel;
                app_state.target_rot.y += e.motion.xrel;
            }
        }
        break;

        case SDL_MOUSEBUTTONDOWN: {
            if (e.button.button == SDL_BUTTON_LEFT) {
                app_state.ui_state.mouse_down = true;
                app_state.ui_state.mouse_down_this_frame = true;
            }
        }
        break;

        case SDL_MOUSEBUTTONUP: {
            if (e.button.button == SDL_BUTTON_LEFT) {
                app_state.ui_state.mouse_down = false;
            }
        }
        break;

        default:
            ;
        }

        switch (e.window.event)
        {
        case SDL_WINDOWEVENT_CLOSE:
            app_state.running = false;

        case SDL_WINDOWEVENT_ENTER:
            app_state.ui_state.mouse_in_window = true;
            break;

        case SDL_WINDOWEVENT_LEAVE:
            app_state.ui_state.mouse_in_window = false;
            break;
        default:
            break;
        }
    }
}

static void draw_scene(application_state & app_state, SDL_Surface* screen_surface)
{
    // animate the model when we aren't manually rotating it
    if(!app_state.ui_state.mouse_down){
        app_state.target_rot.y = ((app_state.gl_state.culm_dt / 20000) - (int)((app_state.gl_state.culm_dt / 20000))) * 360;
        app_state.target_rot.x = abs(cos(app_state.gl_state.culm_dt / 10000)) * 25 + 10;
    }

    //apply model transform
    app_state.gl_state.model_view = look_at(
                                        app_state.gl_state.eye,
                                        app_state.gl_state.center,
                                        app_state.gl_state.up
                                    )
                                    * rot_x(app_state.target_rot.x)
                                    * rot_y(app_state.target_rot.y)
                                    * trans(app_state.target_trans);

    //render the model
    draw_model(*app_state.active_model, app_state.gl_state, *app_state.active_shader);
}

struct bit_scan_result
{
    bool found;
    int index;
};

static bit_scan_result find_least_significant_set_bit(const int value)
{
    bit_scan_result result{};

#if COMPILER_MSVC
    result.Found = _BitScanForward(reinterpret_cast<unsigned long*>(&result.Index), Value);
#else
    for (auto test = 0; test < 32; test++)
    {
        if (value & (1 << test))
        {
            result.index = test;
            result.found = true;

            break;
        }
    }
#endif

    return result;
}

static void copy_frame_buffer_to_screen(application_state& app_state, SDL_Surface* screen_surface)
{
    auto& frame_buffer = app_state.gl_state.output_buffers.frame_buffer;

    auto* frame_pixels = reinterpret_cast<rgba*>(app_state.gl_state.output_buffers.frame_buffer.data);
    auto* target = static_cast<unsigned int*>(screen_surface->pixels);

    const auto r_shift = find_least_significant_set_bit(screen_surface->format->Rmask);
    const auto b_shift = find_least_significant_set_bit(screen_surface->format->Bmask);
    const auto g_shift = find_least_significant_set_bit(screen_surface->format->Gmask);

    assert(r_shift.found);
    assert(b_shift.found);
    assert(g_shift.found);

    for (auto y = 0; y < frame_buffer.height; y++) {
        for (auto x = 0; x < frame_buffer.width; x++) {
            const auto old_pixel = *frame_pixels++;

            //don't blit alpha, sdl screen surface doesn't let us anyway
            const int r = old_pixel.r;
            const int g = old_pixel.g;
            const int b = old_pixel.b;

            *target++ = r << r_shift.index | g << g_shift.index | b << b_shift.index;
        }
    }
}