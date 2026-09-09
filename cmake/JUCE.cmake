# Resolves the JUCE framework.
#
# Priority:
#   1. -DANTIMATR_JUCE_PATH=/path/to/JUCE   (explicit local checkout)
#   2. FETCHCONTENT_SOURCE_DIR_JUCE          (standard CMake FetchContent override)
#   3. FetchContent download of the pinned tag below.
#
# The pinned version is the only version the project is validated against.
set(ANTIMATR_JUCE_TAG "8.0.9" CACHE STRING "JUCE git tag to fetch when no local JUCE is provided")
set(ANTIMATR_JUCE_PATH "" CACHE PATH "Optional path to a local JUCE checkout")

# Never build JUCE's own extras/examples.
set(JUCE_BUILD_EXTRAS   OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

if (ANTIMATR_JUCE_PATH AND EXISTS "${ANTIMATR_JUCE_PATH}/CMakeLists.txt")
    message(STATUS "ANTI-MATR: using local JUCE at ${ANTIMATR_JUCE_PATH}")
    add_subdirectory("${ANTIMATR_JUCE_PATH}" "${CMAKE_BINARY_DIR}/juce-build" EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${ANTIMATR_JUCE_TAG}
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE)
    message(STATUS "ANTI-MATR: fetching JUCE ${ANTIMATR_JUCE_TAG} (override with -DANTIMATR_JUCE_PATH or FETCHCONTENT_SOURCE_DIR_JUCE)")
    FetchContent_MakeAvailable(JUCE)
endif()
