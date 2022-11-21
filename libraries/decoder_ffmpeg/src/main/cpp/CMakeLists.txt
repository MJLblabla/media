cmake_minimum_required(VERSION 3.18.1)
# Declares and names the project.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
project("ffmpegJNI")

set(jnilibs ${CMAKE_SOURCE_DIR}/../jniLibs)
set(libname ffmpegJNI)

include_directories(
        include
        videodecoder
)

link_directories(
        ${jnilibs}/${ANDROID_ABI}
)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.


file(GLOB src-files
        ${CMAKE_SOURCE_DIR}/*.cc
        ${CMAKE_SOURCE_DIR}/videodecoder/*.cc
        )

add_library( # Sets the name of the library.
        ${libname}

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${src-files}
        )

#

set(third-party-libs_ffmpeg
        avcodec
        swresample
        avutil
        swscale
        )

set(native-libs
        log
        android
        )
target_link_libraries( # Specifies the target library.
        ${libname}
        ${third-party-libs_ffmpeg}
        ${native-libs}
        )
