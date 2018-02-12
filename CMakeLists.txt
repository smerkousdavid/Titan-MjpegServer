cmake_minimum_required(VERSION 2.8.11)

project(mjpegserver)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake_modules")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREAD ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json" )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
		${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
	)
endif()

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

find_package(Boost 1.50.3 REQUIRED COMPONENTS date_time filesystem chrono thread atomic wave coroutine system iostreams serialization locale random)

if (Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIR})
endif()

find_package(OpenCV 3.2.0 REQUIRED)

if (OpenCV_FOUND)
	include_directories(${OpenCV_INCLUDE_DIR})
endif()

find_package(CppRestSdk REQUIRED)

if (CppRestSdk_FOUND)
	include_directories(${CppRestSdk_INCLUDE_DIR})
endif()


#add_library(mjpegserver SHARED ${SOURCE_FILES})
add_executable(mjpegserver ${SOURCE_FILES})

target_include_directories(mjpegserver PUBLIC "include/")
target_link_libraries(mjpegserver LINK_PUBLIC ${Boost_LIBRARIES} ${OpenCV_LIBRARIES} ${CppRestSdk_LIBRARIES})

