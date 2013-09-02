cmake_minimum_required(VERSION 2.8)
# eventracer/util CMAKE

INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/base)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/input)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/races)

SET(CMAKE_C_FLAGS "-Wno-long-long -std=c99")
SET(CMAKE_CXX_FLAGS "-Wno-long-long")

SET(EVENTRACER_FILTERS_H
    CallTraceBuilder.h EventGraphInfo.h GraphFix.h TimerGraph.h)
SET(EVENTRACER_FILTERS_CPP
    CallTraceBuilder.cpp EventGraphInfo.cpp GraphFix.cpp TimerGraph.cpp)

ADD_LIBRARY(eventracer_util ${EVENTRACER_FILTERS_H} ${EVENTRACER_FILTERS_CPP})
TARGET_LINK_LIBRARIES(eventracer_util eventracer_input eventracer_races base util gflags.a)
