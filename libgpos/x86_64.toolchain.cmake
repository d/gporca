# Copyright (c) 2015, Pivotal Software, Inc.

set(GPOS_ARCH_BITS "64")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64" CACHE STRING "c++ flags")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64" CACHE STRING "c flags")

set(FORCED_CMAKE_SYSTEM_PROCESSOR "x86_64")
