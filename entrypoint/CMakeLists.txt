project(shooter-entrypoint)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE shooter-shared)

find_path(HIREDIS_HEADER hiredis)
target_include_directories(${PROJECT_NAME} PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
target_link_libraries(${PROJECT_NAME} PRIVATE ${HIREDIS_LIB})

find_path(REDIS_PLUS_PLUS_HEADER sw)
target_include_directories(${PROJECT_NAME} PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
target_link_libraries(${PROJECT_NAME} PRIVATE ${REDIS_PLUS_PLUS_LIB})
