cmake_minimum_required(VERSION 3.10)
project(CropSense-Face-Detection)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT DEFINED OpenCV_DIR)
    set(OpenCV_DIR "C:/libraries/opencv/build")
    message(STATUS "Using default OpenCV_DIR: ${OpenCV_DIR}")
endif()

if(NOT EXISTS "${OpenCV_DIR}/OpenCVConfig.cmake")
    message(FATAL_ERROR "OpenCVConfig.cmake not found in ${OpenCV_DIR}")
endif()
find_package(OpenCV QUIET)

if(NOT OpenCV_FOUND)
    message(FATAL_ERROR
        "OpenCV not found!\n"
        "Check these:\n"
        "1. OpenCV_DIR is correct\n"
        "2. OpenCV is built (lib folder exists)\n"
        "3. Architecture matches (x64 vs x86)\n"
    )
endif()

add_executable(main
    src/main.cpp
    src/user_input.cpp
    src/image_processing.cpp
)
include(FetchContent)
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)
target_include_directories(main PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(main
    ${OpenCV_LIBS}
    nlohmann_json::nlohmann_json
)
