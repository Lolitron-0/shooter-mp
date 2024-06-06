#include "Player.hpp"
#include "Scene.hpp"
#include "SessionOptions.hpp"
#include "Wall.hpp"
#include <cstdint>
#include <raylib.h>

static void DebugOutput(ESteamNetworkingSocketsDebugOutputType eType,
                        const char* pszMsg)
{
    printf(" %s\n", pszMsg);
    fflush(stdout);
    if (eType == k_ESteamNetworkingSocketsDebugOutputType_Bug)
    {
        fflush(stdout);
        fflush(stderr);
        std::abort();
    }
}

auto main() -> int
{

    SteamDatagramErrMsg errMsg;
    if (!GameNetworkingSockets_Init(nullptr, errMsg))
    {
        std::cerr << "GameNetworkingSockets_Init failed.  " << errMsg << '\n';
    }
    SteamNetworkingUtils()->SetDebugOutputFunction(
        k_ESteamNetworkingSocketsDebugOutputType_Msg, DebugOutput);

    smp::game::Scene scene;

    InitWindow(smp::game::SessionOptions::WorldWidth,
               smp::game::SessionOptions::WorldHeight, "my game client hehehe");

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        scene.Update();

        BeginDrawing();
        ClearBackground(RAYWHITE);
        scene.Draw();
        EndDrawing();
    }

    CloseWindow();
}
