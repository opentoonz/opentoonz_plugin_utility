cmake_minimum_required(VERSION 2.4)
project(opentoonz_plugin_utility)

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
endif(WIN32)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++")
endif(APPLE)

find_package(OpenCV REQUIRED)
set(LIBS ${OpenCV_LIBS})

include_directories(opentoonz_plugin_headers include "${OpenCV_INCLUDE_DIRS}")
link_directories("${OpenCV_LIBS}")

set(HEADERS
    include/toonz_utility.hpp)

set(SOURCES
	src/lib.cpp)

set(LIBNAME opentoonz_plugin_utility)

add_library(${LIBNAME} STATIC ${HEADERS} ${SOURCES})

set_target_properties(${LIBNAME} PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

if(WIN32)
	set_target_properties(${LIBNAME} PROPERTIES
		PREFIX "lib"
		SUFFIX ".lib")
endif(WIN32)

if(MAC)
	set_target_properties(${LIBNAME} PROPERTIES
		PREFIX "lib"
		SUFFIX ".a")
endif(MAC)

target_link_libraries(${LIBNAME} ${LIBS})
