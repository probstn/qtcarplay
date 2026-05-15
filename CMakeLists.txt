cmake_minimum_required(VERSION 3.16)

project(QtCarplay VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Quick Multimedia)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavutil libswscale)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(appQtCarplay
    main.cpp
    src/CarplayController.cpp
    src/CarplayController.h
    src/CarplayProtocol.cpp
    src/CarplayProtocol.h
    src/H264Decoder.cpp
    src/H264Decoder.h
    src/PcmRingBuffer.cpp
    src/PcmRingBuffer.h
    src/UsbDongleTransport.cpp
    src/UsbDongleTransport.h
)

qt_add_qml_module(appQtCarplay
    URI QtCarplay
    NO_CACHEGEN
    QML_FILES
        Main.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appQtCarplay PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appQtCarplay
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appQtCarplay
    PRIVATE
        Qt6::Quick
        Qt6::Multimedia
        ${LIBUSB_LIBRARIES}
        ${FFMPEG_LIBRARIES}
)

target_include_directories(appQtCarplay
    PRIVATE
        ${LIBUSB_INCLUDE_DIRS}
        ${FFMPEG_INCLUDE_DIRS}
)

target_link_directories(appQtCarplay
    PRIVATE
        ${LIBUSB_LIBRARY_DIRS}
        ${FFMPEG_LIBRARY_DIRS}
)

install(TARGETS appQtCarplay
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(carplay_capture_selftest
    tools/capture_selftest.cpp
    src/H264Decoder.cpp
    src/H264Decoder.h
)

target_include_directories(carplay_capture_selftest
    PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
)

target_link_directories(carplay_capture_selftest
    PRIVATE
        ${FFMPEG_LIBRARY_DIRS}
)

target_link_libraries(carplay_capture_selftest
    PRIVATE
        Qt6::Multimedia
        ${FFMPEG_LIBRARIES}
)

add_executable(carplay_usb_probe_selftest
    tools/usb_probe_selftest.cpp
)

target_include_directories(carplay_usb_probe_selftest
    PRIVATE
        ${LIBUSB_INCLUDE_DIRS}
)

target_link_directories(carplay_usb_probe_selftest
    PRIVATE
        ${LIBUSB_LIBRARY_DIRS}
)

target_link_libraries(carplay_usb_probe_selftest
    PRIVATE
        ${LIBUSB_LIBRARIES}
)
