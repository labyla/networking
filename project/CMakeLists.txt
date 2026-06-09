cmake_minimum_required(VERSION 3.20)

project(opengl-engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third_party)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(app)