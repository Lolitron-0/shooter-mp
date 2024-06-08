project(shooter-client)

find_package(raylib 5.0 REQUIRED)

# Dependencies

add_executable(
  ${PROJECT_NAME}
  src/main.cpp
  src/Player.cpp
  src/Bullet.cpp
  src/Wall.cpp
  src/Scene.cpp
  src/GameObject.cpp
  src/NetworkClient.cpp)

# target_link_libraries(${PROJECT_NAME} PUBLIC raylib)

target_link_libraries(${PROJECT_NAME} PRIVATE shooter-shared)

target_link_libraries(${PROJECT_NAME} PUBLIC EnTT)
