#include "cartridge.h"
#include "cpu.h"
#include "timer.h"
#include "bus.h"
#include <iostream>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_opengl3.h>

static const uint32_t dmg_palette[4] = {
    0xFFFFFFFF, // white
    0xFFAAAAAA, // light gray
    0xFF555555, // dark gray
    0xFF000000, // black
};
bool key_state[8] = {false,false,false,false, false,false,false,false};

int main(int argc, char* argv[]){

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-rom.gb>\n";
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "Error: SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_Window* window = SDL_CreateWindow(
        "Gameboy Emulator", // title
        SDL_WINDOWPOS_CENTERED, // x position
        SDL_WINDOWPOS_CENTERED, // y position
        800, 600, // width and height
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "Error: SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // 1 = enable vsync

    IMGUI_CHECKVERSION(); // check DLL/API version match
    ImGui::CreateContext(); // allocate internal state
    ImGui::StyleColorsDark(); // Choose a dark color scheme
    
    // Hook imgui into SDL2 (for inputs) and OpenGL3 (for rendering)
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    const std::string path = argv[1];
    Cartridge cart;

    if(!cart.loadRom(path)){
        std::cerr << "Failed to load ROM" << std::endl;
        return 1;
    }

    Bus bus(cart);
    CPU cpu(bus);

    GLuint gb_tex = 0;
    std::vector<uint32_t> gpu_frame(160 * 144);

    glGenTextures(1, &gb_tex);
    glBindTexture(GL_TEXTURE_2D, gb_tex);
    // allocate empty RGBA buffer for 160×144
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 160, 144, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // nearest neighbor scaling so pixels stay sharp
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    bool running = true;
    while (running)
    {
        // poll events feed them to imgui and watch for quit
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                bool down = (event.type == SDL_KEYDOWN);
                switch (event.key.keysym.sym) {
                    case SDLK_z:          key_state[0] = down; break; // A
                    case SDLK_x:          key_state[1] = down; break; // B
                    case SDLK_BACKSPACE:  key_state[2] = down; break; // Select
                    case SDLK_RETURN:     key_state[3] = down; break; // Start
                    case SDLK_RIGHT:      key_state[4] = down; break;
                    case SDLK_LEFT:       key_state[5] = down; break;
                    case SDLK_UP:         key_state[6] = down; break;
                    case SDLK_DOWN:       key_state[7] = down; break;
                }
            }
        }

        // start new imgui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        std::cout << "[CPU A] " << std::hex << int(cpu.A) << std::endl;
        std::cerr << "LCDC=" << std::hex << int(bus.ppu.getLCDC())
          << "  STAT=" << int(bus.ppu.getSTAT()) << "\n";
        // — Emulator: run until VBlank —
        while (!bus.ppu.isFrameReady()) {
            int m = cpu.step();
            bus.step(m * 4, cpu);
        }
        bus.ppu.clearNewFrameFlag();

        static int frameCounter = 0;
        std::cout << "frameCounter" << frameCounter << std::endl;
        if (++frameCounter == 1000) {
            std::cerr << "[HACK] Forcing LCDC to 0x91\n";
            bus.write(0xFF40, 0x91);
        }
        if(frameCounter >= 1000){
            std::cout << "[POST HACK] LCDC" << std::hex << int(bus.ppu.getLCDC()) << std::endl;
        }


        // — Upload frame to GPU texture —
        // 1) Grab the 0–3 indices from the PPU:
        const uint8_t* idx_buf = bus.ppu.getFrameBuffer();

        // 2) Expand via your dmg_palette into a uint32_t RGBA buffer:
        for (int i = 0; i < 160*144; i++) {
            gpu_frame[i] = dmg_palette[idx_buf[i] & 3];
        }

        // 3) Upload the 160×144×4‐byte RGBA image:
        glBindTexture(GL_TEXTURE_2D, gb_tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0,
                        0, 0, 160, 144,
                        GL_RGBA, GL_UNSIGNED_BYTE,
                        reinterpret_cast<const GLvoid*>(gpu_frame.data()));
        glBindTexture(GL_TEXTURE_2D, 0);

        // draw within ImGui
        ImGui::Begin("Game Boy Screen");
        ImGui::Image((void*)(intptr_t)gb_tex,
                     ImVec2(160 * 2.0f, 144 * 2.0f));
        ImGui::End();

        // render ImGui to OpenGL
        ImGui::Render();
        int w, h;
        SDL_GL_GetDrawableSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}