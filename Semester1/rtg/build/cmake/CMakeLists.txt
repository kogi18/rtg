cmake_minimum_required(VERSION 2.8)

include("add_configuration.cmake")
include("add_subdirectory_if_exists.cmake")

project(rtg)

add_configuration(Submission Release)

if (WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
	add_definitions(-DGLCORE_STATIC)
else ()
	set(CMAKE_C_FLAGS "-std=c90")
	set(CMAKE_CXX_FLAGS "-std=c++11")
endif ()

add_subdirectory(framework)
add_subdirectory_if_exists(task1)
add_subdirectory_if_exists(task2)
add_subdirectory_if_exists(task3)
add_subdirectory_if_exists(task4)
add_subdirectory_if_exists(task5)
add_subdirectory_if_exists(reference_solutions)
add_subdirectory_if_exists(demos)
