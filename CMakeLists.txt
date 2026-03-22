cmake_minimum_required(VERSION 3.16)
project(CropSense CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- OpenCV ----------------------------------------------------------------
find_package(OpenCV REQUIRED)

# ---- Sources ---------------------------------------------------------------
add_executable(CropSense
    main.cpp
    image_processing.cpp
    user_input.cpp
)

target_include_directories(CropSense PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(CropSense PRIVATE ${OpenCV_LIBS})

# ---- Threads ---------------------------------------------------------------
find_package(Threads REQUIRED)
target_link_libraries(CropSense PRIVATE Threads::Threads)

# ---- Windows shell shortcut (optional) ------------------------------------
if(WIN32)
    target_compile_definitions(CropSense PRIVATE _WIN32_WINNT=0x0601)
endif()
