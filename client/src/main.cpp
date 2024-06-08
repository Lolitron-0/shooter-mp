#include "NetworkClient.hpp"
#include "Player.hpp"
#include "Scene.hpp"
#include "SessionOptions.hpp"
#include "Wall.hpp"
#include "steam/steamnetworkingtypes.h"
#include <cassert>
#include <cstdint>
#include <memory>
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

    auto networkClient{ std::make_unique<smp::network::NetworkClient>() };
    networkClient->FindFreeRoom("127.0.0.1:32232");

    smp::game::Scene scene{ std::move(networkClient) };

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
