cmake_minimum_required(VERSION 2.8)
# eventracer/webapp CMAKE

INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/base)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/util)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/input)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/races)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/filters)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/util)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/webapp)

SET(CMAKE_C_FLAGS "-Wno-long-long -std=c99")
SET(CMAKE_CXX_FLAGS "-Wno-long-long")

SET(RACEA_WEBAPP_H
    TraceReorder.h)
SET(RACEA_WEBAPP_CPP
    TraceReorder.cpp)

ADD_LIBRARY(eventracer_webera ${RACEA_WEBAPP_H} ${RACEA_WEBAPP_CPP})
TARGET_LINK_LIBRARIES(eventracer_webera base util eventracer_races eventracer_filters gflags.a pthread dl)

ADD_EXECUTABLE(webera WebERAMain.cpp)
TARGET_LINK_LIBRARIES(webera eventracer_webapp eventracer_webera)


