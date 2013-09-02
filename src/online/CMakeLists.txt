cmake_minimum_required(VERSION 2.8)
# online CMAKE

INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/base)

SET(ONLINERACE_H
    VCRaceDetector.h)
SET(ONLINERACE_CPP
    VCRaceDetector.cpp)

ADD_LIBRARY(onlinerace ${ONLINERACE_H} ${ONLINERACE_CPP})
TARGET_LINK_LIBRARIES(onlinerace base)

INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/online)

ADD_EXECUTABLE(vctest VCRaceDetectorTest.cpp)
TARGET_LINK_LIBRARIES(vctest onlinerace)

