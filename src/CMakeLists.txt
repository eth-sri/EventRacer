cmake_minimum_required(VERSION 2.8)

PROJECT(WEB)

ADD_DEFINITIONS("-Wall -pedantic")

ADD_SUBDIRECTORY(base)
ADD_SUBDIRECTORY(util)

ADD_SUBDIRECTORY(eventracer/input)
ADD_SUBDIRECTORY(eventracer/races)
ADD_SUBDIRECTORY(eventracer/filters)
ADD_SUBDIRECTORY(eventracer/util)
ADD_SUBDIRECTORY(eventracer/tool)
ADD_SUBDIRECTORY(eventracer/trialwebapp)
ADD_SUBDIRECTORY(eventracer/webapp)



ADD_SUBDIRECTORY(online)
