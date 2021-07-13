cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)

get_filename_component(
    LIBRARY_NAME 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    NAME
)

project(${LIBRARY_NAME})

message(
    STATUS 
    "Configuring module ${LIBRARY_NAME}..."
)

# # # # # # # # # # # # # # # # # Dependency # # # # # # # # # # # # # # # #  
# VMTK
find_package(VMTK REQUIRED)
include(${VMTK_USE_FILE})

# VTK
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

# VTK
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

# Boost
FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED )
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

# Finding needed source files.
file(
    GLOB
    LIBRARY_SRC
    *.c*
)
# Finding needed header files.
file(
    GLOB
    LIBRARY_INC
    *.h*
)

# # # # # # # # # # # # # # # # # BUILD LIBRARY # # # # # # # # # # # # # # # #  
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
set(LIB_TYPE SHARED)

add_library(
    ${LIBRARY_NAME}
    ${LIB_TYPE}
    ${LIBRARY_SRC}
    ${LIBRARY_INC}
)

target_link_libraries(
    ${LIBRARY_NAME}
    ${VTK_LIBRARIES}
    ${ITK_LIBRARIES}
    ${VMTK_LIBRARIES}
)

set_target_properties(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#Grouping in MSVC
source_group(
    "Header Files" 
    FILES 
    ${LIBRARY_INC}
)
source_group(
    "Source Files" 
    FILES 
    ${LIBRARY_SRC}
)

MESSAGE(STATUS "Configuring module ${LIBRARY_NAME} finished!")