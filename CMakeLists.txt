cmake_minimum_required(VERSION 3.15)

project(WorldGuard CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        endstone
        GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
        GIT_TAG v0.6.1
)
FetchContent_MakeAvailable(endstone)

FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG master
)
FetchContent_MakeAvailable(yaml-cpp)

endstone_add_plugin(${PROJECT_NAME} src/Guard.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE resources)
target_link_libraries(${PROJECT_NAME} PUBLIC yaml-cpp::yaml-cpp)