cmake_minimum_required(VERSION 2.8)
# eventracer/filters CMAKE

INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/base)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/input)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/races)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/util)

SET(CMAKE_C_FLAGS "-Wno-long-long -std=c99")
SET(CMAKE_CXX_FLAGS "-Wno-long-long")

SET(EVENTRACER_FILTERS_H
    RaceTags.h)
SET(EVENTRACER_FILTERS_CPP
    RaceTags.cpp)

ADD_LIBRARY(eventracer_filters ${EVENTRACER_FILTERS_H} ${EVENTRACER_FILTERS_CPP})
TARGET_LINK_LIBRARIES(eventracer_filters eventracer_input eventracer_races eventracer_util base util gflags.a)