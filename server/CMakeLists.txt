project(shooter-server)

add_executable(${PROJECT_NAME} src/main.cpp src/GameServer.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE shooter-shared)

target_link_libraries(${PROJECT_NAME} PUBLIC GameNetworkingSockets
                                             GameNetworkingSockets::static)

# game state storage
target_link_libraries(${PROJECT_NAME} PUBLIC EnTT)

# # unfortunately there is no raymath only target, physics engine would be #
# separate anyways target_link_libraries(${PROJECT_NAME} PUBLIC raylib)
