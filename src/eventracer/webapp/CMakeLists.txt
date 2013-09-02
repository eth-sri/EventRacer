cmake_minimum_required(VERSION 2.8)
# eventracer/webapp CMAKE

INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/base)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/util)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/input)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/races)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/filters)
INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/eventracer/util)

SET(CMAKE_C_FLAGS "-Wno-long-long -std=c99")
SET(CMAKE_CXX_FLAGS "-Wno-long-long")

SET(RACEA_WEBAPP_H
    ActionLogPrint.h EventGraphViz.h GraphViz.h HTMLTable.h JsViewer.h RaceApp.h mongoose.h)
SET(RACEA_WEBAPP_CPP
    ActionLogPrint.cpp EventGraphViz.cpp GraphViz.cpp HTMLTable.cpp JsViewer.cpp RaceApp.cpp mongoose.c)

ADD_LIBRARY(eventracer_webapp ${RACEA_WEBAPP_H} ${RACEA_WEBAPP_CPP})
TARGET_LINK_LIBRARIES(eventracer_webapp base util eventracer_races eventracer_filters gflags.a pthread dl)

ADD_EXECUTABLE(raceanalyzer RaceAnalyzerMain.cpp)
TARGET_LINK_LIBRARIES(raceanalyzer eventracer_webapp)


