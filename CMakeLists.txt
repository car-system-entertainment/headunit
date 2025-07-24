cmake_minimum_required(VERSION 3.10)
project(headunit)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Detectar arquitetura
if(NOT ARCH)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(ARCH arm)
    else()
        set(ARCH x64)
    endif()
endif()

# Definir o diretório base do código-fonte
if(DEFINED HEADUNIT_DIR)
    set(HEADUNIT_PATH "${HEADUNIT_DIR}")
else()
    set(HEADUNIT_PATH "${CMAKE_SOURCE_DIR}")
endif()

# Incluir dependências
find_package(PkgConfig REQUIRED)
pkg_check_modules(
    PKG_DEPS REQUIRED
    gstreamer-1.0 
    gstreamer-app-1.0 
    gstreamer-video-1.0 
    gstreamer-audio-1.0 
    gstreamer-codecparsers-1.0
    libcrypto 
    libusb-1.0 
    openssl 
    glib-2.0
    gtk+-3.0 
    x11 
    protobuf 
    sdl2 
    libudev 
    alsa 
    libpulse
    libpulse-simple
)

include_directories(${PKG_DEPS_INCLUDE_DIRS} ${HEADUNIT_PATH}/hu ${HEADUNIT_PATH}/common ${HEADUNIT_PATH}/hu/generated.${ARCH})
link_directories(${PKG_DEPS_LIBRARY_DIRS})

# Definir flags de compilação
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic -pthread -Wno-unused-parameter -Wno-narrowing")

# Definir os arquivos-fonte comuns (tanto para a lib quanto para o executável)
set(SOURCES
        ${HEADUNIT_PATH}/hu/hu_aap.cpp
        ${HEADUNIT_PATH}/hu/hu_aad.cpp
        ${HEADUNIT_PATH}/hu/hu_ssl.cpp
        ${HEADUNIT_PATH}/hu/hu_usb.cpp
        ${HEADUNIT_PATH}/hu/hu_uti.cpp
        ${HEADUNIT_PATH}/hu/hu_tcp.cpp
        ${HEADUNIT_PATH}/hu/generated.${ARCH}/hu.pb.cc
        ${HEADUNIT_PATH}/common/audio.cpp
        ${HEADUNIT_PATH}/common/glib_utils.cpp
        ${HEADUNIT_PATH}/common/command_server.cpp
        ${HEADUNIT_PATH}/common/web++/web++.cpp
        ${HEADUNIT_PATH}/bt/ub_bluetooth.cpp
        ${HEADUNIT_PATH}/outputs.cpp
        ${HEADUNIT_PATH}/callbacks.cpp
        ${HEADUNIT_PATH}/server.cpp
)

# Gerar arquivos Protocol Buffers
find_package(Protobuf REQUIRED)
add_custom_command(
        OUTPUT ${HEADUNIT_PATH}/hu/generated.${ARCH}/hu.pb.cc ${HEADUNIT_PATH}/hu/generated.${ARCH}/hu.pb.h
        COMMAND protobuf::protoc ARGS --proto_path=${HEADUNIT_PATH}/hu/ --cpp_out=${HEADUNIT_PATH}/hu/generated.${ARCH}/ ${HEADUNIT_PATH}/hu/hu.proto
        DEPENDS ${HEADUNIT_PATH}/hu/hu.proto
)
add_custom_target(proto DEPENDS ${HEADUNIT_PATH}/hu/generated.${ARCH}/hu.pb.cc ${HEADUNIT_PATH}/hu/generated.${ARCH}/hu.pb.h)

# Criar a biblioteca se este projeto for incluído como dependência
if (DEFINED HEADUNIT_DIR)
    message(STATUS "Compile headunit as library")
    add_library(headunit_lib STATIC ${SOURCES})
    target_link_libraries(headunit_lib ${PKG_DEPS_LIBRARIES} dl pthread)
    add_dependencies(headunit_lib proto)
else()
    list(APPEND SOURCES ${HEADUNIT_PATH}/main.cpp)
    message(STATUS "Compile headunit as executable")
    add_executable(headunit ${SOURCES})
    target_link_libraries(headunit ${PKG_DEPS_LIBRARIES} dl pthread)
    add_dependencies(headunit proto)
endif()
