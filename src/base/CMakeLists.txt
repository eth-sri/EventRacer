cmake_minimum_required(VERSION 2.8)
# base CMAKE

SET(BASE_H
    base.h
    file.h
    mutex.h
    stringprintf.h
    strutil.h
    system_error.h)
SET(BASE_CPP
    base.cpp
    file.cpp
    mutex.cpp
    stringprintf.cpp
    strutil.cpp)

ADD_LIBRARY(base ${BASE_H} ${BASE_CPP})
