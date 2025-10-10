

cmake_minimum_required(VERSION 3.10)

project(perg VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Add executable
add_executable( perg
        src/main.cpp
        src/perg.cpp
)

