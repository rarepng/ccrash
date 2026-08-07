cmake_minimum_required(VERSION 4.0)
project(rview2 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)

list(APPEND CMAKE_CXX_SOURCE_FILE_EXTENSIONS cu)

if(NOT CUDA_PATH OR NOT CUDA_ARCH)
    message(FATAL_ERROR "Please define CUDA_PATH and CUDA_ARCH in CMakePresets.json")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    execute_process(
        COMMAND "${CMAKE_CXX_COMPILER}" "-print-resource-dir"
        OUTPUT_VARIABLE CLANG_RESOURCE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
    )
    file(TO_CMAKE_PATH "${CLANG_RESOURCE_DIR}" CLANG_RESOURCE_DIR)
endif()


set(CPM_PATH "${CMAKE_BINARY_DIR}/_deps/CPM.cmake")
if(EXISTS "${CPM_PATH}")
    file(SIZE "${CPM_PATH}" _cpm_size)
    if(_cpm_size EQUAL 0)
        file(REMOVE "${CPM_PATH}")
    endif()
endif()
if(NOT EXISTS "${CPM_PATH}")
    file(DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake
        "${CPM_PATH}"
        STATUS status
    )
    list(GET status 0 code)
    if(NOT code EQUAL 0)
        file(REMOVE "${CPM_PATH}")
        message(FATAL_ERROR "CPM download failed. Cleaned up empty file.")
    endif()
endif()
include(${CPM_PATH})
if(NOT COMMAND CPMAddPackage)
    message(FATAL_ERROR "CPM broke again :(")
endif()

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
    set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "build types & other ghosts" FORCE)
endif()



set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)

file(GLOB_RECURSE PROJECT_HEADERS "include/*.hpp")
file(GLOB PROJECT_SOURCES "src/main.cu")
set(OTHER_SOURCES
)

set_source_files_properties(${PROJECT_SOURCES} PROPERTIES LANGUAGE CXX) # $&#@ u nvcc

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


file(GLOB CUDART_DLL "${CUDA_PATH}/bin/x64/cudart64_*.dll")
if(NOT CUDART_DLL)
    message(FATAL_ERROR "Could not find the cudart64 dll in ${CUDA_PATH}/bin")
endif()
list(GET CUDART_DLL 0 CUDART_DLL_PATH)


add_executable(rview2 ${OTHER_SOURCES} ${PROJECT_SOURCES} ${PROJECT_HEADERS})

include(CMakeLocal.txt OPTIONAL)


target_link_directories(rview2 PRIVATE
    "${CUDA_PATH}/lib/x64"

)

target_compile_definitions(rview2 PRIVATE
    "_NV_RSQRT_SPECIFIER="
)


target_link_libraries(rview2
    PRIVATE
    # cudart
    "${CUDART_DLL_PATH}"
)

target_include_directories(rview2
    BEFORE
    PUBLIC
        "${PROJECT_BINARY_DIR}/generated/include" # in bin just in case of cross compiling
        "${PROJECT_SOURCE_DIR}/include"
        "${CUDA_PATH}/include"
)


target_compile_options(rview2 PUBLIC

    $<$<CXX_COMPILER_ID:Clang>:--target=x86_64-w64-windows-gnu> # and god is dead
    $<$<CXX_COMPILER_ID:Clang>:-resource-dir=${CLANG_RESOURCE_DIR}>
    $<$<CXX_COMPILER_ID:Clang>:-x;cuda>
    $<$<CXX_COMPILER_ID:Clang>:--cuda-gpu-arch=${CUDA_ARCH}>
    $<$<CXX_COMPILER_ID:Clang>:--cuda-path=${CUDA_PATH}>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-unknown-cuda-version>

    "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>,$<CONFIG:sanitize>>:-g>"
    "$<$<CONFIG:Debug>:-Wall;-Wextra;-Wunused-variable>"

    # temporary
    "$<$<CONFIG:Debug>:-Wno-unused-parameter>"
    "$<$<CONFIG:Debug>:-Wno-missing-field-initializers>"

)
target_link_options(rview2 PUBLIC
    "$<$<CONFIG:sanitize>:-static-libasan>"
    "$<$<CONFIG:sanitize>:-fsanitize=address>"
)
target_link_options(rview2 PRIVATE
    # no idea here..
    # "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>,$<CONFIG:sanitize>>:-static>"

    # "-static"
    "-static-libgcc" "-static-libstdc++"
)

target_link_libraries(rview2 PRIVATE
    "${CUDART_DLL_PATH}"
    $<$<BOOL:${UNIX}>:c;pthread>
    $<$<BOOL:${WIN32}>:pthread;DbgHelp;winmm;iphlpapi>
)

add_custom_command(TARGET rview2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CUDART_DLL_PATH}"
    "$<TARGET_FILE_DIR:rview2>"
)


install(TARGETS rview2 RUNTIME DESTINATION $<CONFIG> CONFIGURATIONS Release Distribution BleedingEdge MinSizeRel)

# if(WIN32)
#     get_filename_component(RVIEW2_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

#     install(CODE "
#         message(STATUS \"resolving dynamic libraries for $<TARGET_FILE_NAME:rview2>...\")
        
#         file(GET_RUNTIME_DEPENDENCIES
#             EXECUTABLES \"\${CMAKE_INSTALL_PREFIX}/$<CONFIG>/$<TARGET_FILE_NAME:rview2>\"
#             DIRECTORIES \"${RVIEW2_BIN_DIR}\"
#             RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
#             UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
#             PRE_EXCLUDE_REGEXES 
#                 \"api-ms-.*\" 
#                 \"ext-ms-.*\" 
#                 \".*[Ss]ystem32.*\" 
#                 \".*[Ss]ys[Ww][Oo][Ww]64.*\" 
#                 \".*[Ww]in[Ss]x[Ss].*\"
#         )

#         foreach(DEP \${RESOLVED_DEPS})
#             get_filename_component(DEP_NAME \${DEP} NAME)
#             string(TOLOWER \${DEP_NAME} LOWER_DEP_NAME)
            
#             if(NOT LOWER_DEP_NAME MATCHES \"^(kernel32|kernelbase|user32|msvcrt|msvcp.*|advapi32|ws2_32|shell32|setupapi|ole32|oleaut32|gdi32|imm32|version|winmm|crypt32|dbghelp|iphlpapi|vulkan-1|ntdll|combase|sechost|rpcrt4|win32u|cfgmgr32|d3d11|d3d12|dxgi)\\\\.dll$\")
#                 message(STATUS \"deploying \${DEP_NAME}\")
#                 file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/$<CONFIG>\" TYPE SHARED_LIBRARY FILES \"\${DEP}\")
#             endif()
#         endforeach()
#     "
#     CONFIGURATIONS Release Distribution BleedingEdge MinSizeRel)
#     # maybe ill setup a launcher pattern
# endif()

