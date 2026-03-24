cmake_minimum_required(VERSION 3.10)
project(CropSense-Face-Detection)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- OpenCV setup ----
if(WIN32)
    if(NOT DEFINED OpenCV_DIR)
        set(OpenCV_DIR "C:/libraries/opencv/build")
        message(STATUS "Using default OpenCV_DIR: ${OpenCV_DIR}")
    endif()
    if(NOT EXISTS "${OpenCV_DIR}/OpenCVConfig.cmake")
        message(FATAL_ERROR "OpenCVConfig.cmake not found in ${OpenCV_DIR}")
    endif()
    find_package(OpenCV QUIET PATHS "${OpenCV_DIR}")
else()
    find_package(OpenCV REQUIRED)
endif()

if(NOT OpenCV_FOUND)
    message(FATAL_ERROR
        "OpenCV not found!\n"
        "Check:\n"
        "- OpenCV_DIR=${OpenCV_DIR}\n"
        "- Make sure x64/vc16/lib exists\n"
    )
endif()

# ---- Executable ----
add_executable(main
    src/main.cpp
    src/user_input.cpp
    src/image_processing.cpp
)

# ---- JSON (nlohmann) ----
include(FetchContent)
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# ---- Linking ----
target_link_libraries(main
    PRIVATE
    ${OpenCV_LIBS}
    nlohmann_json::nlohmann_json
)

# ---- Debug info ----
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
