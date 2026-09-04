# ============================================================================
# rt_reader GUI —— Qt6 Widgets + liblsl(风格对齐 Monitoring_interface)
#
# 依赖你的 Qt6 Desktop: /opt/Qt/6.10.3/gcc_64
# 命令行构建:
#   cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/Qt/6.10.3/gcc_64
#   cmake --build build
# 或在 Qt Creator 中:选 Desktop Qt 6.10.3 kit,打开本 CMakeLists.txt
# ============================================================================
cmake_minimum_required(VERSION 3.19)
project(rt_reader_gui LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(rt_reader_gui
    main.cpp
    mainwindow.cpp
    wavewidget.cpp
    ../lsl_receiver.cpp
)

target_include_directories(rt_reader_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..          # lsl_receiver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include  # lsl_cpp.h / lsl_c.h
)

# 直接链接本仓库内的 liblsl,并让可执行文件运行时能找到它
target_link_libraries(rt_reader_gui PRIVATE
    Qt6::Widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/liblsl.so
)

set_target_properties(rt_reader_gui PROPERTIES
    BUILD_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/../lib"
)
