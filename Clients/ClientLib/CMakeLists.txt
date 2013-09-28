project(SphereSim_ClientLib)
cmake_minimum_required(VERSION 2.8)

set (PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set (PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(USE_DOUBLE 0)
add_definitions("-DUSE_DOUBLE=${USE_DOUBLE}")

include_directories("${PROJECT_INCLUDE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_BINARY_DIR}")

set(SOURCES ${SOURCES}
	${PROJECT_SOURCE_DIR}/ActionSender.cpp
	${PROJECT_SOURCE_DIR}/FrameBuffer.cpp
)
set(HEADERS ${HEADERS}
	${PROJECT_INCLUDE_DIR}/ActionSender.hpp
	${PROJECT_INCLUDE_DIR}/FrameBuffer.hpp
)

add_library(SphereSim_ClientLib ${SOURCES} ${HEADERS})
target_link_libraries(SphereSim_ClientLib)
qt5_use_modules(SphereSim_ClientLib Core Network)