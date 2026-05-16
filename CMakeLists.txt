cmake_minimum_required(VERSION 3.22)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/.macros.m4" VMGJ_VERSION_M4)
string(REGEX MATCH "VMGJ_VERSION_STRING,[ \t]*([0-9]+\\.[0-9]+\\.[0-9]+)" _vmgj_version_match
    "${VMGJ_VERSION_M4}"
)
if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR "Unable to read VMGJ version from .macros.m4")
endif()

project(VMGJ
    VERSION ${CMAKE_MATCH_1}
    DESCRIPTION "Verificatum Multiplicative Groups library for Java"
    LANGUAGES C
)

set(VMGJ_ABI_VERSION 1)

# Keep multi-config generators such as Visual Studio from scattering
# related runtime, library, and import-library artifacts unpredictably.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

# Options
option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_TESTING "Build Java/native test target" ON)
option(ENABLE_SANITIZERS "Enable sanitizers for Debug builds" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# ====================== Coverage Support ======================
if(ENABLE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(STATUS "Coverage enabled - using --coverage flags")
        add_compile_options(--coverage -fprofile-abs-path)
        add_link_options(--coverage)
    else()
        message(WARNING "Coverage not supported on this compiler")
    endif()
endif()

# Dependencies
find_package(Java REQUIRED COMPONENTS Development)
find_package(JNI REQUIRED)

find_path(GMP_INCLUDE_DIR
    NAMES gmp.h
    DOC "Directory containing gmp.h"
)
find_library(GMP_LIBRARY
    NAMES gmp libgmp
    DOC "GMP library"
)
if(NOT GMP_INCLUDE_DIR OR NOT GMP_LIBRARY)
    message(FATAL_ERROR "GMP not found. Set GMP_INCLUDE_DIR and GMP_LIBRARY.")
endif()

find_path(GMPMEE_INCLUDE_DIR
    NAMES gmpmee.h
    DOC "Directory containing gmpmee.h"
)
find_library(GMPMEE_LIBRARY
    NAMES gmpmee libgmpmee libgmpmee-1
    PATH_SUFFIXES lib bin
    DOC "GMPMEE library"
)
if(NOT GMPMEE_LIBRARY AND WIN32)
    find_file(GMPMEE_LIBRARY
        NAMES libgmpmee-1.dll gmpmee.dll
        PATH_SUFFIXES bin lib
        DOC "GMPMEE runtime library"
    )
endif()
if(NOT GMPMEE_INCLUDE_DIR OR NOT GMPMEE_LIBRARY)
    message(FATAL_ERROR "GMPMEE not found. Set GMPMEE_INCLUDE_DIR and GMPMEE_LIBRARY.")
endif()

add_library(GMP::GMP UNKNOWN IMPORTED)
set_target_properties(GMP::GMP PROPERTIES
    IMPORTED_LOCATION "${GMP_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
)

add_library(GMPMEE::GMPMEE UNKNOWN IMPORTED)
set_target_properties(GMPMEE::GMPMEE PROPERTIES
    IMPORTED_LOCATION "${GMPMEE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${GMPMEE_INCLUDE_DIR}"
)

# Java sources
set(VMGJ_GENERATED_JAVA_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/java")
set(VMGJ_GENERATED_PACKAGE_DIR "${VMGJ_GENERATED_JAVA_DIR}/com/verificatum/vmgj")
set(VMGJ_GENERATED_VMG_JAVA "${VMGJ_GENERATED_PACKAGE_DIR}/VMG.java")
set(VMGJ_GENERATED_MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/MANIFEST.MF")
set(VMGJ_CLASSES_DIR "${CMAKE_CURRENT_BINARY_DIR}/classes")
set(VMGJ_JNI_HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/native")
set(VMGJ_JAR "${CMAKE_CURRENT_BINARY_DIR}/verificatum-vmgj-${PROJECT_VERSION}.jar")

set(VMGJ_JAVA_SOURCES
    "${VMGJ_GENERATED_VMG_JAVA}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/java/com/verificatum/vmgj/TestVMG.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/java/com/verificatum/vmgj/BenchVMG.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/java/com/verificatum/vmgj/FpowmTab.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/java/com/verificatum/vmgj/MillerRabin.java"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/java/com/verificatum/vmgj/package-info.java"
)

add_custom_command(
    OUTPUT "${VMGJ_GENERATED_VMG_JAVA}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${VMGJ_GENERATED_PACKAGE_DIR}"
    COMMAND "${CMAKE_COMMAND}"
            -DINPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/src/java/com/verificatum/vmgj/VMG.magic
            -DOUTPUT_FILE=${VMGJ_GENERATED_VMG_JAVA}
            -DVMGJ_VERSION=${PROJECT_VERSION}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVMGJava.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/java/com/verificatum/vmgj/VMG.magic"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVMGJava.cmake"
    VERBATIM
)

add_custom_target(vmgj_generated_java DEPENDS "${VMGJ_GENERATED_VMG_JAVA}")

add_custom_command(
    OUTPUT "${VMGJ_GENERATED_MANIFEST}"
    COMMAND "${CMAKE_COMMAND}"
            -DINPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/src/mf/MANIFEST.MF.src
            -DOUTPUT_FILE=${VMGJ_GENERATED_MANIFEST}
            -DVMGJ_VERSION=${PROJECT_VERSION}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVMGJava.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/mf/MANIFEST.MF.src"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVMGJava.cmake"
    VERBATIM
)

add_custom_target(vmgj_generated_manifest DEPENDS "${VMGJ_GENERATED_MANIFEST}")

add_custom_command(
    OUTPUT
        "${VMGJ_CLASSES_DIR}/com/verificatum/vmgj/VMG.class"
        "${VMGJ_JNI_HEADER_DIR}/com_verificatum_vmgj_VMG.h"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${VMGJ_CLASSES_DIR}" "${VMGJ_JNI_HEADER_DIR}"
    COMMAND "${Java_JAVAC_EXECUTABLE}"
            -Xlint:unchecked
            -h "${VMGJ_JNI_HEADER_DIR}"
            -d "${VMGJ_CLASSES_DIR}"
            ${VMGJ_JAVA_SOURCES}
    DEPENDS ${VMGJ_JAVA_SOURCES}
    VERBATIM
)

add_custom_command(
    OUTPUT "${VMGJ_JAR}"
    COMMAND "${Java_JAR_EXECUTABLE}"
            cfm "${VMGJ_JAR}" "${VMGJ_GENERATED_MANIFEST}"
            -C "${VMGJ_CLASSES_DIR}" com
    DEPENDS
        "${VMGJ_CLASSES_DIR}/com/verificatum/vmgj/VMG.class"
        "${VMGJ_GENERATED_MANIFEST}"
    VERBATIM
)

add_custom_target(vmgj_jar ALL DEPENDS "${VMGJ_JAR}")
add_custom_target(vmgj_jni_headers DEPENDS "${VMGJ_JNI_HEADER_DIR}/com_verificatum_vmgj_VMG.h")

# Main native library
add_library(vmgj
    native/com_verificatum_vmgj_VMG.c
    native/convert.c
    native/convert.h
    "${VMGJ_JNI_HEADER_DIR}/com_verificatum_vmgj_VMG.h"
)

add_dependencies(vmgj vmgj_jni_headers)

target_include_directories(vmgj PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/native"
    "${VMGJ_JNI_HEADER_DIR}"
    ${JNI_INCLUDE_DIRS}
)

target_link_libraries(vmgj PRIVATE
    GMPMEE::GMPMEE
    GMP::GMP
    ${JNI_LIBRARIES}
)

set_target_properties(vmgj PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${VMGJ_ABI_VERSION}
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_compile_options(vmgj PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Werror -fstack-protector-strong>
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<AND:$<CONFIG:Release>,$<C_COMPILER_ID:GNU,Clang,AppleClang>>:-O3>
    $<$<AND:$<CONFIG:Release>,$<C_COMPILER_ID:MSVC>>:/O2>
)

# Sanitizers
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(vmgj PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(vmgj PRIVATE -fsanitize=address,undefined)
    endif()
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS vmgj
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES "${VMGJ_JAR}"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/java
)

# Testing
if(BUILD_TESTING)
    enable_testing()
    add_test(
        NAME vmgj_basic
        COMMAND "${Java_JAVA_EXECUTABLE}" -ea
                "-Djava.library.path=$<TARGET_FILE_DIR:vmgj>"
                -classpath "${VMGJ_JAR}"
                com.verificatum.vmgj.TestVMG 256
    )
    set_tests_properties(vmgj_basic PROPERTIES DEPENDS "vmgj;vmgj_jar")
endif()

message(STATUS "VMGJ ${PROJECT_VERSION} configured")
