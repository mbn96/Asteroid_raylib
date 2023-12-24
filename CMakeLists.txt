cmake_minimum_required(VERSION 3.28)
project(Asteroids)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# add_compile_options(-Wall -pthread)
add_compile_options(-pthread)
add_link_options(-pthread -ldl -lm)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_compile_options(-O0)
  add_compile_definitions("MBN_DEBUG")
else()
  add_compile_options(-O3)
endif()

set(M_SRC "./src/main.cpp;")

add_executable(${CMAKE_PROJECT_NAME} ${M_SRC})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "raylib")
