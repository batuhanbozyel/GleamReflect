include(${CMAKE_SOURCE_DIR}/CMake/CMakeUtils.cmake)

cmake_minimum_required(VERSION 3.20)

project(GleamReflect)

    #preventing in source build
    if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
        message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt.")
    endif()

    #enable c++20
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED True)
    set(CMAKE_CXX_EXTENSIONS OFF)

    #check the compiler
    MESSAGE(STATUS "compiler id ${CMAKE_CXX_COMPILER_ID} ${MSVC}")
    # using Clang
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -g3 -O0 -Wall -pedantic -Wextra -m64 -mavx2 -mfma -ffast-math")
    # using Visual Studio C++
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS}  /std:c++20 /W4 /arch:AVX2 /fp:fast /MP /DNOMINMAX")
        add_compile_options(/wd4251 /wd4275 /wd4099 /wd4204)
    endif()

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)

    # GleamReflect project
    file(GLOB_RECURSE SOURCE_FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    )
    file(GLOB_RECURSE INCLUDE_FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    )
    add_library(GleamReflect INTERFACE ${INCLUDE_FILES})
    target_include_directories(GleamReflect INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    option(BUILD_CPP_REFLECTION "Build the CppReflection executable" OFF)
    if (BUILD_CPP_REFLECTION)
        if (WIN32)
            set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT CppReflection)
        elseif (APPLE)
            set(CMAKE_OSX_ARCHITECTURES "arm64")
            set(CMAKE_XCODE_GENERATE_SCHEME true)
            set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY XCODE_SCHEME_EXECUTABLE CppReflection)
        endif()

        # Configure LLVM
        set(LLVM_ENABLE_PROJECTS "clang;clang-tools-extra" CACHE STRING "List of LLVM projects to build")
        find_package(LLVM REQUIRED CONFIG)
        find_package(Clang REQUIRED CONFIG)

        list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR}/../clang)
        include(AddLLVM)
        include(AddClang)

        llvm_map_components_to_libnames(LLVM_LIBS support core frontendopenmp)
        message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
        message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
        message(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
        message(STATUS "LLVM_LIBRARY_DIR: ${LLVM_LIBRARY_DIR}")
        message(STATUS "LLVM_LIBS: ${LLVM_LIBS}")

        add_executable(CppReflection ${SOURCE_FILES} ${INCLUDE_FILES})
        target_link_directories(CppReflection PRIVATE ${LLVM_LIBRARY_DIR})
        target_link_libraries(CppReflection PRIVATE ${LLVM_LIBS}
            clangAST
            clangASTMatchers
            clangBasic
            clangFrontend
            clangSerialization
            clangTooling
        )
        target_include_directories(CppReflection PRIVATE ${LLVM_INCLUDE_DIRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
        )
        SET_WORKING_DIRECTORY(CppReflection ${CMAKE_SOURCE_DIR})

        source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})

        if (APPLE)
            set_target_properties(AArch64TargetParserTableGen
                acc_gen
                ARMTargetParserTableGen
                clang-tablegen-targets
                intrinsics_gen
                omp_gen
                RISCVTargetParserTableGen PROPERTIES FOLDER Dependencies)
        else()
            set_target_properties(AArch64TargetParserTableGen
                acc_gen
                ARMTargetParserTableGen
                clang-tablegen-targets
                intrinsics_gen
                omp_gen
                RISCVTargetParserTableGen
                vt_gen PROPERTIES FOLDER Dependencies)
        endif()
    endif()