# Tencent is pleased to support the open source community by making xLua available.
# Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
# Licensed under the MIT License (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
# http://opensource.org/licenses/MIT
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

cmake_minimum_required(VERSION 2.8)

if ( WIN32 AND NOT CYGWIN AND NOT ( CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" ) AND NOT ANDROID)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT" CACHE STRING "")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd" CACHE STRING "")
endif ()

project(XLua)

set(CMAKE_CXX_STANDARD 17)

if ( IOS )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fembed-bitcode")
endif ()

option ( UINT_ESPECIALLY "using custom ulong" OFF )
option ( GC64 "using gc64" OFF )
option ( LUAC_COMPATIBLE_FORMAT "compatible format" OFF )

find_path(XLUA_PROJECT_DIR NAMES SConstruct
    PATHS 
    ${CMAKE_SOURCE_DIR}
    NO_DEFAULT_PATH
    )

MARK_AS_ADVANCED(XLUA_PROJECT_DIR)

if (NOT LUA_VERSION)
    set(LUA_VERSION "5.4.1")
endif()

set(LUA_SRC_PATH lua-${LUA_VERSION}/src)
set ( LUA_IDSIZE 120 CACHE STRING "gives the maximum size for the description of the source." )

configure_file ( ${LUA_SRC_PATH}/luaconf.h.in ${CMAKE_CURRENT_BINARY_DIR}/luaconf.h )

# common headers
list(APPEND THIRDPART_INC  fmt/include)
list(APPEND THIRDPART_INC  luakit/include)

add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-DWOLFSSL_LIB)
add_definitions(-DWOLFSSL_SRTP)
add_definitions(-DWOLFSSL_NO_SOCK)
add_definitions(-DWOLFSSL_USER_IO)
add_definitions(-DWOLFSSL_USER_SETTINGS)

if (APPLE)
add_definitions(-DWOLFSSL_APPLE_NATIVE_CERT_VALIDATION)
set(CMAKE_LD_FLAGS "-framework CoreFoundation")
set(CMAKE_LD_FLAGS "-framework Security")
endif ()

#begin luapb
set (LUAPB_SRC luapb/src/luapb.cpp)
set (THIRDPART_SRC ${THIRDPART_SRC} ${LUAPB_SRC})
#end luapb

#begin ljson
set (LJSON_SRC ljson/src/ljson.cpp ljson/src/yyjson.c)
set (THIRDPART_SRC ${THIRDPART_SRC} ${LJSON_SRC})
#end ljson

#begin lcodec
set (LCODEC_SRC lcodec/src/lcodec.cpp)
set (THIRDPART_SRC ${THIRDPART_SRC} ${LCODEC_SRC})
#end lcodec

#begin ltimer
set (LTIMER_SRC ltimer/src/ltimer.cpp)
 set (THIRDPART_SRC ${THIRDPART_SRC} ${LTIMER_SRC})
#end ltimer

#begin lminiz
set (LMINIZ_SRC lminiz/src/lminiz.cpp lminiz/src/miniz.c)
set (THIRDPART_SRC ${THIRDPART_SRC} ${LMINIZ_SRC})
#end lminiz

#begin lstdfs
set (LSTDFS_SRC lstdfs/src/lstdfs.cpp)
set (THIRDPART_SRC ${THIRDPART_SRC} ${LSTDFS_SRC})
#end lstdfs

#begin luabus
file(GLOB_RECURSE LUABUS_SRC luabus/src/*.cpp)
set (THIRDPART_SRC ${THIRDPART_SRC} ${LUABUS_SRC})
#end luabus

#begin lss
set(SSL_PATH lssl/src/)
file(GLOB_RECURSE LSSL_SRC ${SSL_PATH}ssl/*.cpp ${SSL_PATH}ssl/*.c ${SSL_PATH}wolfcrypt/src/*.c ${SSL_PATH}src/*.c)
file(GLOB_RECURSE LSSL_SRC1 ${SSL_PATH}src/x509_str.c ${SSL_PATH}src/x509.c ${SSL_PATH}src/conf.c ${SSL_PATH}src/bio.c ${SSL_PATH}src/pk.c)
file(GLOB_RECURSE LSSL_SRC2 ${SSL_PATH}src/ssl_bn.c ${SSL_PATH}src/ssl_misc.c ${SSL_PATH}src/ssl_asn1.c ${SSL_PATH}src/ssl_crypto.c ${SSL_PATH}src/ssl_certman.c)
file(GLOB_RECURSE LSSL_SRC3 ${SSL_PATH}wolfcrypt/src/evp.c ${SSL_PATH}wolfcrypt/src/misc.c)
list(REMOVE_ITEM LSSL_SRC ${LSSL_SRC1})
list(REMOVE_ITEM LSSL_SRC ${LSSL_SRC2})
list(REMOVE_ITEM LSSL_SRC ${LSSL_SRC3})
list(APPEND THIRDPART_INC  lssl/src)
set (THIRDPART_SRC ${THIRDPART_SRC} ${LSSL_SRC})
#end lssl

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${LUA_SRC_PATH}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${THIRDPART_INC}
)

aux_source_directory(${LUA_SRC_PATH} LUA_CORE)
list(REMOVE_ITEM LUA_CORE ${LUA_SRC_PATH}/lua.c ${LUA_SRC_PATH}/luac.c)
set ( LUA_LIB )

# set ( LUA_CORE ${LUA_SRC_PATH}/lapi.c ${LUA_SRC_PATH}/lcode.c ${LUA_SRC_PATH}/lctype.c ${LUA_SRC_PATH}/ldebug.c ${LUA_SRC_PATH}/ldo.c ${LUA_SRC_PATH}/ldump.c
#   ${LUA_SRC_PATH}/lfunc.c ${LUA_SRC_PATH}/lgc.c ${LUA_SRC_PATH}/llex.c ${LUA_SRC_PATH}/lmem.c ${LUA_SRC_PATH}/lobject.c ${LUA_SRC_PATH}/lopcodes.c ${LUA_SRC_PATH}/lparser.c
#   ${LUA_SRC_PATH}/lstate.c ${LUA_SRC_PATH}/lstring.c ${LUA_SRC_PATH}/ltable.c ${LUA_SRC_PATH}/ltm.c ${LUA_SRC_PATH}/lundump.c ${LUA_SRC_PATH}/lvm.c ${LUA_SRC_PATH}/lzio.c )
# set ( LUA_LIB ${LUA_SRC_PATH}/lauxlib.c ${LUA_SRC_PATH}/lbaselib.c ${LUA_SRC_PATH}/lbitlib.c ${LUA_SRC_PATH}/lcorolib.c ${LUA_SRC_PATH}/ldblib.c
#   ${LUA_SRC_PATH}/liolib.c ${LUA_SRC_PATH}/lmathlib.c ${LUA_SRC_PATH}/loslib.c ${LUA_SRC_PATH}/lstrlib.c ${LUA_SRC_PATH}/ltablib.c ${LUA_SRC_PATH}/linit.c
#   ${LUA_SRC_PATH}/lutf8lib.c ${LUA_SRC_PATH}/loadlib.c )

set ( XLUA_CORE i64lib.c xlua.c memory_leak_checker.c)

macro(source_group_by_dir proj_dir source_files)
    if(MSVC OR APPLE)
        get_filename_component(sgbd_cur_dir ${proj_dir} ABSOLUTE)
        foreach(sgbd_file ${${source_files}})
			get_filename_component(sgbd_abs_file ${sgbd_file} ABSOLUTE)
            file(RELATIVE_PATH sgbd_fpath ${sgbd_cur_dir} ${sgbd_abs_file})
            string(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
            string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
            string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
            if(sgbd_nogroup)
                set(sgbd_group_name "\\")
            endif(sgbd_nogroup)
            source_group(${sgbd_group_name} FILES ${sgbd_file})
        endforeach(sgbd_file)
    endif(MSVC OR APPLE)
endmacro(source_group_by_dir)

source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} LUA_CORE)
source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} LUA_LIB)
source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} XLUA_CORE)

if (APPLE)
    if (IOS)
        ADD_DEFINITIONS(-DLUA_USE_IOS) # Despite claiming to be ISO C, iOS does not implement 'system'.(iOS11)
        set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)")
        add_library(xlua STATIC
           ${LUA_CORE}
           ${LUA_LIB}
           ${XLUA_CORE}
           ${THIRDPART_SRC}
        )
		set_xcode_property (xlua IPHONEOS_DEPLOYMENT_TARGET "7.0" "all")
    else ()
        ADD_DEFINITIONS(-DLUA_USE_MACOSX) #osx platform emmylua debugger must have this option or can not load cpath
        if (BUILD_SILICON)
           set(CMAKE_OSX_ARCHITECTURES arm64)

            add_library(xlua SHARED
                ${LUA_CORE}
                ${LUA_LIB}
                ${XLUA_CORE}
                ${THIRDPART_SRC}
            )
        else ()
            set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_64_BIT)")
            add_library(xlua MODULE
                ${LUA_CORE}
                ${LUA_LIB}
                ${XLUA_CORE}
                ${THIRDPART_SRC}
            )
            set_target_properties ( xlua PROPERTIES BUNDLE TRUE )
            #set_target_properties ( xlua PROPERTIES FRAMEWORK TRUE )
            #set_target_properties ( xlua PROPERTIES MACOSX_RPATH TRUE )
        endif ()
    endif ()
elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Switch")
    add_library(xlua STATIC
        ${LUA_CORE}
        ${LUA_LIB}
        ${XLUA_CORE}
        ${THIRDPART_SRC}
    )
    target_compile_options(xlua PRIVATE -m64 -mcpu=cortex-a57+fp+simd+crypto+crc -fno-common -fno-short-enums -ffunction-sections -fdata-sections -fPIC -fms-extensions)
else ( )
    add_library(xlua SHARED
        ${LUA_CORE}
        ${LUA_LIB}
        ${XLUA_CORE}
        ${THIRDPART_SRC}
    )
endif ( )

if ( WIN32 AND NOT CYGWIN )
    target_compile_definitions (xlua PRIVATE LUA_BUILD_AS_DLL)
endif ( )

if (GC64 AND USING_LUAJIT)
    target_compile_definitions (xlua PRIVATE LUAJIT_ENABLE_GC64)
endif ()

if (LUAC_COMPATIBLE_FORMAT AND NOT USING_LUAJIT)
    target_compile_definitions (xlua PRIVATE LUAC_COMPATIBLE_FORMAT)
endif ()

if(UINT_ESPECIALLY)
    ADD_DEFINITIONS(-DUINT_ESPECIALLY)
endif()
	
if ( WIN32 AND NOT CYGWIN )
    target_link_libraries(xlua
        ws2_32
        ${THIRDPART_LIB} 
    )
else ( )
    target_link_libraries(xlua
        m
        ${THIRDPART_LIB} 
    )
endif ( )
