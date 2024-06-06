project(shooter-shared)

add_library(${PROJECT_NAME} src/Components.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC src/)

target_link_libraries(${PROJECT_NAME} PUBLIC Boost::program_options)

target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)

# linking it all just to get Vectors again
target_link_libraries(${PROJECT_NAME} PUBLIC raylib)
