cmake_minimum_required(VERSION 2.8)
# eventracer/races CMAKE

INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/base)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/util)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/input)

SET(CMAKE_C_FLAGS "-Wno-long-long -std=c99")
SET(CMAKE_CXX_FLAGS "-Wno-long-long")

SET(RACES_H
    BitClocks.h
    EventGraph.h
    ThreadMapping.h
    VarsInfo.h)
SET(RACES_CPP
	BitClocks.cpp
    EventGraph.cpp
    ThreadMapping.cpp
    VarsInfo.cpp)

ADD_LIBRARY(eventracer_races ${RACES_H} ${RACES_CPP})
TARGET_LINK_LIBRARIES(eventracer_races eventracer_input base util gflags.a)