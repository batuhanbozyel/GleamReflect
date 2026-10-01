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
    if (MSVC)
        target_sources(GleamReflect INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/GleamReflect.natvis>)
    endif()

    option(BUILD_CPP_REFLECTION "Build the CppReflection executable" OFF)
    if (BUILD_CPP_REFLECTION)
        if (WIN32)
            set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT CppReflection)
        elseif (APPLE)
            set(CMAKE_OSX_ARCHITECTURES "arm64")
            set(CMAKE_XCODE_GENERATE_SCHEME true)
            set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY XCODE_SCHEME_EXECUTABLE CppReflection)
        endif()

        # Prebuilt LLVM distributions are compiled with zlib/zstd/libxml2 support, so
        # their exported targets reference imported targets that a bare machine does
        # not provide. zlib and zstd are real link dependencies of LLVMSupport, so
        # build them from source when the host has no installation; libxml2 is only
        # referenced through the link interface and never pulled in, so a stub is
        # enough there. All of this has to happen before find_package(LLVM).
        include(FetchContent)

        if (NOT TARGET ZLIB::ZLIB)
            set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
            FetchContent_Declare(zlib
                GIT_REPOSITORY https://github.com/madler/zlib.git
                GIT_TAG v1.3.1
                GIT_SHALLOW TRUE
                EXCLUDE_FROM_ALL
            )
            FetchContent_MakeAvailable(zlib)
            add_library(ZLIB::ZLIB ALIAS zlibstatic)
        endif()

        if (NOT TARGET zstd::libzstd_static)
            set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
            set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
            set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
            set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
            set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL "" FORCE)
            FetchContent_Declare(zstd
                GIT_REPOSITORY https://github.com/facebook/zstd.git
                GIT_TAG v1.5.6
                GIT_SHALLOW TRUE
                SOURCE_SUBDIR build/cmake
                EXCLUDE_FROM_ALL
            )
            FetchContent_MakeAvailable(zstd)
            if (NOT TARGET zstd::libzstd_static)
                add_library(zstd::libzstd_static ALIAS libzstd_static)
            endif()
        endif()

        if (NOT TARGET LibXml2::LibXml2)
            add_library(LibXml2::LibXml2 INTERFACE IMPORTED)
        endif()

        # Configure LLVM
        set(LLVM_ENABLE_PROJECTS "clang;clang-tools-extra" CACHE STRING "List of LLVM projects to build")
        find_package(LLVM REQUIRED CONFIG)
        find_package(Clang REQUIRED CONFIG)

        list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR}/../clang)
        include(AddLLVM)
        include(AddClang)

        # Prefer the combined shared libraries when the LLVM installation ships
        # them. Homebrew's libclangDriver.a holds two distinct members both named
        # AMDGPU.cpp.o (Arch/AMDGPU.cpp and ToolChains/AMDGPU.cpp) and the archive
        # index only records one of them, so linking the static clang libraries
        # leaves the shadowed member's symbols undefined.
        if (TARGET clang-cpp AND TARGET LLVM)
            set(LLVM_LIBS LLVM)
            set(CLANG_LIBS clang-cpp)
        else()
            llvm_map_components_to_libnames(LLVM_LIBS support core frontendopenmp)
            set(CLANG_LIBS
                clangAST
                clangASTMatchers
                clangBasic
                clangFrontend
                clangSerialization
                clangTooling
            )
        endif()

        message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
        message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
        message(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
        message(STATUS "LLVM_LIBRARY_DIR: ${LLVM_LIBRARY_DIR}")
        message(STATUS "LLVM_LIBS: ${LLVM_LIBS}")
        message(STATUS "CLANG_LIBS: ${CLANG_LIBS}")

        add_executable(CppReflection ${SOURCE_FILES} ${INCLUDE_FILES})
        target_link_directories(CppReflection PRIVATE ${LLVM_LIBRARY_DIR})
        target_link_libraries(CppReflection PRIVATE ${LLVM_LIBS} ${CLANG_LIBS} GleamReflect)
        target_include_directories(CppReflection PRIVATE ${LLVM_INCLUDE_DIRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
        )
        SET_WORKING_DIRECTORY(CppReflection ${CMAKE_SOURCE_DIR})

        source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})

        # Tidy up the third party and LLVM/Clang tablegen helper targets into a
        # folder. The tablegen ones only exist when configuring against an LLVM build
        # tree, so skip any that a prebuilt LLVM installation does not define.
        set(DEPENDENCY_TARGETS
            zlibstatic
            zlib
            libzstd_static
            AArch64TargetParserTableGen
            acc_gen
            ARMTargetParserTableGen
            clang-tablegen-targets
            intrinsics_gen
            omp_gen
            RISCVTargetParserTableGen
            vt_gen
        )
        foreach (DEPENDENCY_TARGET ${DEPENDENCY_TARGETS})
            if (TARGET ${DEPENDENCY_TARGET})
                set_target_properties(${DEPENDENCY_TARGET} PROPERTIES FOLDER Dependencies)
            endif()
        endforeach()
    endif()