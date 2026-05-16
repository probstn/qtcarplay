set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

if(NOT DEFINED ENV{RPI_SYSROOT})
    message(FATAL_ERROR "RPI_SYSROOT must point to the Raspberry Pi sysroot")
endif()

set(CMAKE_SYSROOT "$ENV{RPI_SYSROOT}")
set(CMAKE_STAGING_PREFIX "$ENV{RPI_QT_TARGET}")

set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH
    "${CMAKE_SYSROOT}"
    "$ENV{RPI_QT_TARGET}"
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_C_FLAGS_INIT "--sysroot=${CMAKE_SYSROOT}")
set(CMAKE_CXX_FLAGS_INIT "--sysroot=${CMAKE_SYSROOT}")
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "-Wl,-rpath-link,${CMAKE_SYSROOT}/lib/aarch64-linux-gnu -Wl,-rpath-link,${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu"
)
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT}")

set(ENV{PKG_CONFIG_SYSROOT_DIR} "${CMAKE_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR}
    "${CMAKE_SYSROOT}/lib/aarch64-linux-gnu/pkgconfig:${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig:${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig"
)
