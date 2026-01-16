CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(robots)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

SET(VERSION "0.0.0")

STRING(REGEX MATCHALL "([0-9]+)" VERSION_DIGITS "${VERSION}")

LIST(GET VERSION_DIGITS 0 CPACK_PACKAGE_VERSION_MAJOR)
LIST(GET VERSION_DIGITS 1 CPACK_PACKAGE_VERSION_MINOR)
LIST(GET VERSION_DIGITS 2 CPACK_PACKAGE_VERSION_PATCH)

SET(CPACK_PACKAGE_NAME "robots")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Google's robots.txt parser and matcher C++ library")
SET(CPACK_PACKAGE_VENDOR "Google Inc.")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY} ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

SET(base_with_ver "robots-[0-9]+\\\\.[0-9]+\\\\.[0-9]+")
SET(CPACK_SOURCE_IGNORE_FILES
    "/_CPack_Packages/"
    "/CMakeFiles/"
    "/.deps/"
    "^${base_with_ver}(-Source|-Linux)?/"
    "${base_with_ver}.tar\\\\.(gz|bz2|Z|lzma|xz)$"
    "\\\\.o$"
    "~$"
    "/\\\\.svn/"
    "/CMakeCache\\\\.txt$"
    "/CTestTestfile\\\\.cmake$"
    "/cmake_install\\\\.cmake$"
    "/CPackConfig\\\\.cmake$"
    "/CPackSourceConfig\\\\.cmake$"
    "/tags$"
    "^config\\\\.h$"
    "/install_manifest\\\\.txt$"
    "/Testing/"
    "ids-whitelist\\\\.txt"
    "/_Inline/"
    "/(B|build|BUILD)/"
    "/autom4te.cache/"
)

############ build options ##############

OPTION(ROBOTS_BUILD_STATIC "If ON, robots will build also the static library" ON)
OPTION(ROBOTS_BUILD_TESTS "If ON, robots will build test targets" OFF)
OPTION(ROBOTS_INSTALL "If ON, enable the installation of the targets" ON)
OPTION(ROBOTS_SUPPORT_CONTENT_SIGNAL "If ON, enable Content-Signal directive support" ON)

############ helper libs ############

INCLUDE(CPack)
INCLUDE(CheckCCompilerFlag)
INCLUDE(FetchContent)

############ dependencies ##############

FetchContent_Declare(
    ada
    GIT_REPOSITORY https://github.com/ada-url/ada.git
    GIT_TAG main
)
SET(ADA_TESTING OFF CACHE BOOL "" FORCE)
SET(ADA_TOOLS OFF CACHE BOOL "" FORCE)
SET(ADA_BENCHMARKS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ada)

############ compiler flags ##############

IF(MSVC)
    ADD_COMPILE_OPTIONS(/wd4005 /wd4068 /wd4244 /wd4267 /wd4800)
    ADD_DEFINITIONS(/DNOMINMAX /DWIN32_LEAN_AND_MEAN=1 /D_CRT_SECURE_NO_WARNINGS)
ENDIF(MSVC)

SET(COMPILER_FLAGS_TO_CHECK
    "-Wall" "-Werror=implicit-function-declaration"
)

IF(CPU_ARCH)
    LIST(APPEND COMPILER_FLAGS_TO_CHECK "-march=${CPU_ARCH}")
ENDIF(CPU_ARCH)

########### project files ###############

INCLUDE_DIRECTORIES(. ./bindings/c)

######### targets ###########

SET(LIBROBOTS_LIBS)

SET(robots_SRCS ./robots.cc ./bindings/c/robots_c.cc)

ADD_LIBRARY(robots SHARED ${robots_SRCS})
TARGET_LINK_LIBRARIES(robots ada)
TARGET_COMPILE_DEFINITIONS(robots PRIVATE ROBOTS_USE_ADA)
IF(ROBOTS_SUPPORT_CONTENT_SIGNAL)
    TARGET_COMPILE_DEFINITIONS(robots PUBLIC ROBOTS_SUPPORT_CONTENT_SIGNAL=1)
ELSE()
    TARGET_COMPILE_DEFINITIONS(robots PUBLIC ROBOTS_SUPPORT_CONTENT_SIGNAL=0)
ENDIF()
LIST(APPEND LIBROBOTS_LIBS "robots")

IF(ROBOTS_BUILD_STATIC)
    ADD_LIBRARY(robots-static STATIC ${robots_SRCS})
    TARGET_LINK_LIBRARIES(robots-static ada)
    TARGET_COMPILE_DEFINITIONS(robots-static PRIVATE ROBOTS_USE_ADA)
    IF(ROBOTS_SUPPORT_CONTENT_SIGNAL)
        TARGET_COMPILE_DEFINITIONS(robots-static PUBLIC ROBOTS_SUPPORT_CONTENT_SIGNAL=1)
    ELSE()
        TARGET_COMPILE_DEFINITIONS(robots-static PUBLIC ROBOTS_SUPPORT_CONTENT_SIGNAL=0)
    ENDIF()

    LIST(APPEND LIBROBOTS_LIBS "robots-static")

    SET_TARGET_PROPERTIES(robots-static PROPERTIES OUTPUT_NAME "robots")

    SET_TARGET_PROPERTIES(${LIBROBOTS_LIBS} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
ENDIF(ROBOTS_BUILD_STATIC)

IF(WIN_32)
    SET_TARGET_PROPERTIES(robots PROPERTIES DEFINE_SYMBOL DLL_EXPORT)
ENDIF(WIN_32)

ADD_EXECUTABLE(robots-main ./robots_main.cc)
TARGET_LINK_LIBRARIES(robots-main ${LIBROBOTS_LIBS})
SET_TARGET_PROPERTIES(robots-main PROPERTIES OUTPUT_NAME "robots")

############ installation ############

IF(ROBOTS_INSTALL)
    INSTALL(TARGETS ${LIBROBOTS_LIBS}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )

    INSTALL(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/robots.h
        ${CMAKE_CURRENT_SOURCE_DIR}/bindings/c/robots_c.h
        DESTINATION include)

    INSTALL(TARGETS robots-main DESTINATION bin)
ENDIF(ROBOTS_INSTALL)

############ tests ##############

IF(ROBOTS_BUILD_TESTS)
    INCLUDE(CTest)
    ENABLE_TESTING()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
    )
    SET(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    ADD_EXECUTABLE(robots-test ./tests/robots_test.cc)
    TARGET_LINK_LIBRARIES(robots-test ${LIBROBOTS_LIBS} gtest_main)
    TARGET_COMPILE_DEFINITIONS(robots-test PRIVATE ROBOTS_USE_ADA)
    ADD_TEST(NAME robots-test COMMAND robots-test)

    ADD_EXECUTABLE(reporting-robots-test ./tests/reporting_robots_test.cc ./reporting_robots.cc)
    TARGET_LINK_LIBRARIES(reporting-robots-test ${LIBROBOTS_LIBS} gtest_main)
    TARGET_COMPILE_DEFINITIONS(reporting-robots-test PRIVATE ROBOTS_USE_ADA)
    ADD_TEST(NAME reporting-robots-test COMMAND reporting-robots-test)
ENDIF(ROBOTS_BUILD_TESTS)

############ benchmark ##############

OPTION(ROBOTS_BUILD_BENCHMARK "If ON, robots will build benchmark target" OFF)

IF(ROBOTS_BUILD_BENCHMARK)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG main
    )
    SET(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googlebenchmark)

    ADD_EXECUTABLE(robots-benchmark ./tests/robots_benchmark.cc)
    TARGET_LINK_LIBRARIES(robots-benchmark ${LIBROBOTS_LIBS} benchmark::benchmark)
ENDIF(ROBOTS_BUILD_BENCHMARK)
