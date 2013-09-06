cmake_minimum_required(VERSION 2.8)
# util CMAKE

SET(UTIL_H
    Escaping.h UrlEncoding.h)
SET(UTIL_CPP
    Escaping.cpp UrlEncoding.cpp)

INCLUDE_DIRECTORIES(${WEB_SOURCE_DIR}/base)
ADD_LIBRARY(util ${UTIL_H} ${UTIL_CPP})
TARGET_LINK_LIBRARIES(util base)
