#include "EntryServer.hpp"
#include <iostream>
#include <sw/redis++/redis++.h>

using namespace sw;

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

    smp::server::EntryServer server{ "127.0.0.1", 6379 };
    server.Run("127.0.0.1:32232");
}
