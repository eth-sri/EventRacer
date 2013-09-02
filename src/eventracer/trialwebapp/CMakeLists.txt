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
    BrowserRunner.h)
SET(RACEA_WEBAPP_CPP
    BrowserRunner.cpp)

ADD_LIBRARY(eventracer_trialwebapp ${RACEA_WEBAPP_H} ${RACEA_WEBAPP_CPP})
TARGET_LINK_LIBRARIES(eventracer_trialwebapp base util eventracer_races eventracer_filters gflags.a pthread dl)

ADD_EXECUTABLE(web_ra WebRaceAnalyzer.cpp)
TARGET_LINK_LIBRARIES(web_ra eventracer_webapp eventracer_trialwebapp)


