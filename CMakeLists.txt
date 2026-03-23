cmake_minimum_required(VERSION 3.10)
project(CropSense-Face-Detection)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT DEFINED OpenCV_DIR)
    set(OpenCV_DIR "C:\\libraries\\opencv\\build")
endif()
find_package(OpenCV REQUIRED)
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
