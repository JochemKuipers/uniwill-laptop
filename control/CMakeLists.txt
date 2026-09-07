cmake_minimum_required(VERSION 3.16)
project(uniwill-control LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(uniwill-control
	src/main.cpp
	src/window.cpp
)
target_include_directories(uniwill-control PRIVATE src)
target_link_libraries(uniwill-control PRIVATE Qt6::Widgets)

install(TARGETS uniwill-control RUNTIME DESTINATION bin)
install(FILES uniwill-control.desktop DESTINATION share/applications)
