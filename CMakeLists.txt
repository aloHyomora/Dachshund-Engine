cmake_minimum_required(VERSION 3.25)
project(DachshundEngine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UTF-8 Encoding
if(MSVC)
    add_compile_options(/utf-8)
endif()
# Find vcpkg packages in CONFIG mode
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)

add_library(SensorCore
    src/core/sensor/SensorManager.cpp
    src/core/network/NetworkClient.cpp
)

add_executable(dashboard src/dashboard.cpp)    

target_link_libraries(dashboard PRIVATE 
    SensorCore
    glfw
    glad::glad
    imgui::imgui
    implot::implot
)

# Windows에서 Winsock 라이브러리 링크
if(WIN32)
    target_link_libraries(dashboard PRIVATE ws2_32)
endif()

# Linux에서 pthread 및 dl 라이브러리 링크
if(UNIX AND NOT APPLE)
    target_link_libraries(dashboard PRIVATE pthread dl)
endif()

target_include_directories(SensorCore PUBLIC include)
