cmake_minimum_required(VERSION 3.0)
project(RubikRescue)

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(RubikRescue src/main.cpp)

target_link_libraries(RubikRescue ${OpenCV_LIBS})