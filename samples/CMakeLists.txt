cmake_minimum_required(VERSION 2.4)
project(opentoonz_plugin_utility_samples)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(PLATFORM1 32)
        set(PLATFORM2)
    else()
        set(PLATFORM1 64)
        set(PLATFORM2 _64)
    endif()

    if(PLATFORM1 EQUAL 64)
        add_definitions(-Dx64)
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /openmp")
    set(PLUGIN_UTILITY_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../lib/${CMAKE_CFG_INTDIR}/libopentoonz_plugin_utility")
endif(WIN32)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++")
    set(PLUGIN_UTILITY_LIB "${CMAKE_CURRENT_SOURCE_DIR}/../lib/${CMAKE_CFG_INTDIR}/libopentoonz_plugin_utility.a")
endif(APPLE)

find_package(OpenCV REQUIRED)

set(LIBS ${OpenCV_LIBS} ${PLUGIN_UTILITY_LIB})

include_directories(../plugin_sdk/core ../include "${OpenCV_INCLUDE_DIRS}")
link_directories("${OpenCV_LIBS}")

add_subdirectory(amp)
add_subdirectory(blur)
add_subdirectory(snp)
