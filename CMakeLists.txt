cmake_minimum_required(VERSION 3.10)
project(CropSense-Face-Detection)

set(CMAKE_CXX_STANDARD 17)

set(OpenCV_DIR "C:\\libraries\\opencv\\build")

find_package(OpenCV REQUIRED)

add_executable(main src/main.cpp)

target_link_libraries(main ${OpenCV_LIBS})
