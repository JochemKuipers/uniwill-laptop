cmake_minimum_required(VERSION 3.16)
project(uniwill-control LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(uniwill-control
	src/main.cpp
	src/window.cpp
	src/osd.cpp
)
target_include_directories(uniwill-control PRIVATE src)
target_link_libraries(uniwill-control PRIVATE Qt6::Widgets)

include(GNUInstallDirs)
install(TARGETS uniwill-control RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES uniwill-control.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES uniwill-osd.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
# System autostart (absolute so prefix=/usr still lands in /etc/xdg/autostart)
install(FILES uniwill-osd.desktop DESTINATION /etc/xdg/autostart)
