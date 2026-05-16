cmake_minimum_required(VERSION 3.16)

project(QtCarplay VERSION 0.1 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Quick Multimedia)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavutil libswscale)

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
        PkgConfig::LIBUSB
        PkgConfig::FFMPEG
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

target_link_libraries(carplay_capture_selftest
    PRIVATE
        Qt6::Multimedia
        PkgConfig::FFMPEG
)

add_executable(carplay_usb_probe_selftest
    tools/usb_probe_selftest.cpp
)

target_link_libraries(carplay_usb_probe_selftest
    PRIVATE
        PkgConfig::LIBUSB
)

add_executable(carplay_session_probe
    tools/session_probe.cpp
    src/CarplayProtocol.cpp
    src/CarplayProtocol.h
    src/UsbDongleTransport.cpp
    src/UsbDongleTransport.h
)

target_link_libraries(carplay_session_probe
    PRIVATE
        Qt6::Core
        PkgConfig::LIBUSB
)

add_executable(carplay_controller_probe
    tools/controller_probe.cpp
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

target_link_libraries(carplay_controller_probe
    PRIVATE
        Qt6::Core
        Qt6::Multimedia
        PkgConfig::LIBUSB
        PkgConfig::FFMPEG
)

install(TARGETS
    carplay_capture_selftest
    carplay_usb_probe_selftest
    carplay_session_probe
    carplay_controller_probe
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
