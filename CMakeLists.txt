cmake_minimum_required(VERSION 3.25)
project(DachshundEngine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find vcpkg packages in CONFIG mode
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)

add_executable(dashboard src/dashboard.cpp)    

target_link_libraries(dashboard PRIVATE 
    glfw
    glad::glad
    imgui::imgui
    implot::implot
)
