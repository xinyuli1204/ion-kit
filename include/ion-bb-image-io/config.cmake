set(HALIDE_ROOT $ENV{HALIDE_ROOT} CACHE PATH "Path to Halide")
if(HALIDE_ROOT STREQUAL "")
    message(FATAL_ERROR "Set HALIDE_ROOT")
endif()

find_package(OpenCV REQUIRED)

set(INCLUDE_DIRS
    ${HALIDE_ROOT}/include
    ${OpenCV_INCLUDE_DIRS})

set(LINK_DIRS
    ${HALIDE_ROOT}/bin
    ${OpenCV_DIR}/lib)

if (UNIX)
    set(LIBRARIES
        rt
        dl
        pthread
        m
        z
        ${OpenCV_LIBS})
else()
    set(LIBRARIES ${OPENCV_LIBS})
endif()