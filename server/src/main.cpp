#include "GameServer.hpp"
#include "SessionOptions.hpp"
#include <boost/program_options.hpp>
#include <boost/program_options/detail/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <raylib.h>
#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>

SteamNetworkingMicroseconds logTimeZero;

static void DebugOutput(ESteamNetworkingSocketsDebugOutputType eType,
                        const char* pszMsg)
{
    auto time{ SteamNetworkingUtils()->GetLocalTimestamp() - logTimeZero };
    printf("%10.6f %s\n", time * 1e-6, pszMsg);
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
    logTimeZero = SteamNetworkingUtils()->GetLocalTimestamp();

    SteamNetworkingUtils()->SetDebugOutputFunction(
        k_ESteamNetworkingSocketsDebugOutputType_Msg, DebugOutput);

    namespace opts = boost::program_options;
    std::string configPath{};
    std::string ipString{};
    std::string portString{};
    std::string serverName{};

    opts::options_description optsDescription{ "Allowed opitons" };
    // clang-format off
    optsDescription.add_options()
		("help,h", "display help")
		("config,c", 
		 opts::value<std::string>(&configPath)->default_value("./config.json"),
		 "path to server config file")
		("ip,a",
		 opts::value<std::string>(&ipString)->default_value("127.0.0.1"),
        "server ip address")
		("port,p",
		 opts::value<std::string>(&portString)->required(),
        "server port")
		("name,n", opts::value<std::string>(&serverName)->required(),
		 "server name for server discovery");
    // clang-format on

    opts::variables_map vm;
    opts::store(opts::parse_command_line(argc, argv, optsDescription), vm);
    opts::notify(vm);

    if (vm.count("help"))
    {
        std::cout << optsDescription << std::endl;
        return 0;
    }

    std::ifstream configFile{ configPath };
    if (!configFile.is_open())
    {
        std::cout << optsDescription << std::endl;
        std::cerr << "Could not open config!\n";
        return 1;
    }

    nlohmann::json configJson = nlohmann::json::parse(configFile);
    smp::game::SessionOptions sessionOptions{ configJson };
    smp::server::GameServer server{ std::move(sessionOptions) };
    server.Run(ipString + ":" + portString);
}
