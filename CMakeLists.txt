## This file is a quick tutorial on writing CMakeLists for targeting the Vita
cmake_minimum_required(VERSION 3.2)

## This includes the Vita toolchain, must go before project definition
# It is a convenience so you do not have to type
# -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake for cmake. It is
# highly recommended that you include this block for all projects.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

## Define project parameters here
# Name of the project
project(syberia2v)
# This line adds Vita helper macros, must go after project definition in order
# to build Vita specific artifacts (self/vpk).
include("${VITASDK}/share/vita.cmake" REQUIRED)

## Configuration options for this app
# Display name (under bubble in LiveArea)
set(VITA_APP_NAME "Syberia 2 Vita")
# Unique ID must be exactly 9 characters. Recommended: XXXXYYYYY where X =
# unique string of developer and Y = a unique number for this app
set(VITA_TITLEID  "SYB200000")
# Optional version string to show in LiveArea's more info screen
set(VITA_VERSION  "00.01")

## Flags and includes for building
# Note that we make sure not to overwrite previous flags
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -Wl,-q,--wrap,memcpy,--wrap,memmove,--wrap,memset,--allow-multiple-definition -fdiagnostics-color=always -DGNU_SOURCE -O3 -mfloat-abi=softfp")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -Wl,-q,--wrap,memcpy,--wrap,memmove,--wrap,memset,--allow-multiple-definition -DGNU_SOURCE -O3 -mfloat-abi=softfp")
set(CMAKE_C_FLAGS "-fno-optimize-sibling-calls -g -Wl,-q,--wrap,memcpy,--allow-multiple-definition,--wrap,memmove,--wrap,memset -Wno-format-overflow -D_GNU_SOURCE -Wall -O3 -mfloat-abi=softfp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# Optional. You can specify more param.sfo flags this way.
#set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1 -d ATTRIBUTE2=12")
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")

# debug
# set(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_RULE_MESSAGES OFF)

add_subdirectory("lib/kubridge/")
# add_subdirectory("lib/pthread-embedded/")

# # Add any additional include paths here
# include_directories(
#   ../common # This is for debugScreenPrintf(), you shouldn't need it in your projects
# )

# include_directories(
#   lib/pthread-embedded
# )

# Add any additional library paths here
# ${CMAKE_CURRENT_BINARY_DIR} lets you use any library currently being built
link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

## Build and link
# Add all the files needed to compile here
add_executable(${PROJECT_NAME}
  src/main.c
  src/common/debugScreen.c
  src/so_util.c
  src/util.c
  src/dialog.c
  src/log.c
  src/jni.c
  src/sce_pthread.c
)

# Library to link to (drop the -l prefix). This will mostly be stubs.
target_link_libraries(${PROJECT_NAME}
  -Wl,--whole-archive
  pthread
  -Wl,--no-whole-archive

  dl
  openal
  z
  stdc++
  vorbis
  vorbisfile
  ogg
  vitaGL
  m
  vitashark
  SceShaccCgExt
  mathneon
  mpg123
  curl
  ssl
  crypto
  taihen_stub
  kubridge_stub
  SceAppMgr_stub
  SceAppUtil_stub
  SceAudio_stub
  SceAudioIn_stub
  SceCtrl_stub
  SceCommonDialog_stub
  SceDisplay_stub
  SceKernelDmacMgr_stub
  SceFios2_stub
  SceGxm_stub
  SceMotion_stub
  SceNet_stub
  SceShaccCg_stub
  SceSysmodule_stub
  ScePower_stub
  SceTouch_stub
  SceVshBridge_stub
  SceAvPlayer_stub
  SceNpTrophy_stub
)

## Create Vita files
vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
# The FILE directive lets you add additional files to the VPK, the syntax is
# FILE src_path dst_path_in_vpk. In this case, we add the LiveArea paths.
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
