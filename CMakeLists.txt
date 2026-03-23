cmake_minimum_required(VERSION 3.10)
project(CropSense-Face-Detection)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED OpenCV_DIR)
    set(OpenCV_DIR "C:\\libraries\\opencv\\build")
endif()

find_package(OpenCV REQUIRED)
add_executable(main src/main.cpp)
target_link_libraries(main ${OpenCV_LIBS})
