cmake_minimum_required(VERSION 3.10)
project(robotstxt_singleheader_demo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Demo executable (C++ API)
add_executable(demo demo.cpp)

# Demo with reporting (C++ API)
add_executable(demo_reporting demo_reporting.cpp)

# Demo with C API
add_executable(demo_c demo_c.cpp)
