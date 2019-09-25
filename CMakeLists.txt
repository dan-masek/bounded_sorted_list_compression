PROJECT(SO10)
# =============================================================================
CMAKE_MINIMUM_REQUIRED(VERSION 3.13)
#SET(CMAKE_VERBOSE_MAKEFILE ON)
# =============================================================================
IF(WIN32)
  ADD_DEFINITIONS(/W3)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
  ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
ENDIF()
# -----------------------------------------------------------------------------
SET(DEPS_ROOT ${PROJECT_SOURCE_DIR}/deps)
SET(DEPS_ROOT_BIN ${DEPS_ROOT}/bin)
SET(DEPS_ROOT_HEADERS ${DEPS_ROOT}/include)
SET(DEPS_ROOT_LIB ${DEPS_ROOT}/lib)
SET(DEPS_ROOT_SLIB ${DEPS_ROOT}/lib/static)
# -----------------------------------------------------------------------------
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
  ${PROJECT_SOURCE_DIR}/bin/static
)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY
  ${PROJECT_SOURCE_DIR}/bin/dynamic
)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  ${PROJECT_SOURCE_DIR}/bin/runtime
)
# =============================================================================
SET(ROOT src)
# -----------------------------------------------------------------------------
INCLUDE_DIRECTORIES(${ROOT})
# -----------------------------------------------------------------------------
INCLUDE_DIRECTORIES(${DEPS_ROOT_HEADERS})
LINK_DIRECTORIES(${DEPS_ROOT_LIB})
LINK_DIRECTORIES(${DEPS_ROOT_SLIB})
# =============================================================================
LIST(APPEND LIBFASTAC__SRC
  ${ROOT}/fastac/arithmetic_codec.cpp
  
  ${ROOT}/fastac/constants.cpp
  ${ROOT}/fastac/error.cpp
  
  ${ROOT}/fastac/adaptive_bit_model.cpp
  ${ROOT}/fastac/adaptive_data_model.cpp
  ${ROOT}/fastac/adaptive_esc_data_model.cpp
  ${ROOT}/fastac/static_bit_model.cpp
  ${ROOT}/fastac/static_data_model.cpp
)
LIST(APPEND LIBFASTAC__HDR
  ${ROOT}/fastac/arithmetic_codec.hpp
  
  ${ROOT}/fastac/constants.hpp
  ${ROOT}/fastac/error.hpp
  
  ${ROOT}/fastac/adaptive_bit_model.hpp
  ${ROOT}/fastac/adaptive_data_model.hpp
  ${ROOT}/fastac/adaptive_esc_data_model.hpp
  ${ROOT}/fastac/static_bit_model.hpp
  ${ROOT}/fastac/static_data_model.hpp
)
# -----------------------------------------------------------------------------
LIST(APPEND LIBFASTAC__FILES
  ${LIBFASTAC__SRC}
  ${LIBFASTAC__HDR}
)
# =============================================================================
LIST(APPEND SO10__SRC
  ${ROOT}/main.cpp
)
LIST(APPEND SO10__HDR
)
# -----------------------------------------------------------------------------
LIST(APPEND SO10__FILES
  ${SO10__SRC}
  ${SO10__HDR}
)
# =============================================================================
ADD_LIBRARY(libfastac
  ${LIBFASTAC__FILES}
)
# =============================================================================
ADD_EXECUTABLE(so10
  ${SO10__FILES}
)
TARGET_LINK_LIBRARIES(so10
  libfastac
  zlib
  bz2
  snappy64
)
# =============================================================================
SOURCE_GROUP("so10" FILES
  ${SO10__SRC}
  ${SO10__HDR}
)
SOURCE_GROUP("libfastac" FILES
  ${LIBFASTAC__SRC}
  ${LIBFASTAC__HDR}
)
# =============================================================================
