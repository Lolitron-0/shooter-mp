#include "NetworkClient.hpp"
#include "Player.hpp"
#include "Scene.hpp"
#include "SessionOptions.hpp"
#include "Wall.hpp"
#include "steam/steamnetworkingtypes.h"
#include <boost/program_options.hpp>
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

auto main(int argc, char** argv) -> int
{
    SteamDatagramErrMsg errMsg;
    if (!GameNetworkingSockets_Init(nullptr, errMsg))
    {
        std::cerr << "GameNetworkingSockets_Init failed.  " << errMsg << '\n';
    }
    SteamNetworkingUtils()->SetDebugOutputFunction(
        k_ESteamNetworkingSocketsDebugOutputType_Msg, DebugOutput);

    namespace opts = boost::program_options;
    std::string entryPointAddr;
    opts::options_description optsDescription{ "Allowed opitons" };
    // clang-format off
    optsDescription.add_options()
		("help,h", "display help")
		("entry,e", 
		 opts::value<std::string>(&entryPointAddr)->required(),
		 "entry point address");
    // clang-format on

    opts::variables_map vm;
    try
    {
        opts::store(opts::parse_command_line(argc, argv, optsDescription), vm);
        opts::notify(vm);
    }
    catch (const opts::required_option& e)
    {
        std::cout << optsDescription << std::endl;
        std::cout << e.what() << std::endl;
        return 0;
    }

    if (vm.count("help"))
    {
        std::cout << optsDescription << std::endl;
        return 0;
    }

    auto networkClient{ std::make_unique<smp::network::NetworkClient>() };
    networkClient->FindFreeRoom(entryPointAddr);

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
