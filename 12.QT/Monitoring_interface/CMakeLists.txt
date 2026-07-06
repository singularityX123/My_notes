cmake_minimum_required(VERSION 3.19)
project(Monitoring_interface LANGUAGES CXX)

find_package(Qt6 6.10.3 REQUIRED COMPONENTS Core Widgets)

# 查找 FFTW 库
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW REQUIRED fftw3)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts Network)

qt_standard_project_setup()

qt_add_executable(Monitoring_interface
    WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/mainwindow.cpp include/mainwindow.h src/mainwindow.ui
    include/bdfparser.h          src/bdfparser.cpp
    include/signalprocessor.h    src/signalprocessor.cpp
    include/eegprocessingworker.h src/eegprocessingworker.cpp
    include/networkreceiver.h    src/networkreceiver.cpp
    include/mnetopomap.h         src/mnetopomap.cpp
    include/eegdisplayfilter.h   src/eegdisplayfilter.cpp
    include/electrodepositions.h src/electrodepositions.cpp
    include/trialannotation.h    src/trialannotation.cpp
    include/topographyrenderer.h src/topographyrenderer.cpp
    include/chartmanager.h       src/chartmanager.cpp
    include/playbackcontroller.h src/playbackcontroller.cpp
    include/topographyvisualizer.h src/topographyvisualizer.cpp
    include/datapipeline.h       src/datapipeline.cpp
    include/ssvepfbcca.h         src/ssvepfbcca.cpp
)

target_include_directories(Monitoring_interface PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Monitoring_interface
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Charts
        Qt::Network
        ${FFTW_LIBRARIES}
        fftw3
        m
)

include(GNUInstallDirs)

install(TARGETS Monitoring_interface
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Monitoring_interface
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
