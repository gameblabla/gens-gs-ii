PROJECT(libgens-tests-Z80Test)
cmake_minimum_required(VERSION 2.6.0)

# Main binary directory. Needed for git_version.h
INCLUDE_DIRECTORIES(${gens-gs-ii_BINARY_DIR})

# Include the previous directory.
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/../")

# Google Test.
INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIR})

# Copy the BIOS and ZEXDOC/ZEXALL executables to the test directory.
FILE(COPY
	bios.bin
	zexdoc.com
	zexall.com
	DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
	)

# Z80 tests.
ADD_EXECUTABLE(Z80Test Z80Test.cpp)
TARGET_LINK_LIBRARIES(Z80Test cz80 ${GTEST_LIBRARY})
ADD_TEST(NAME Z80Test
	COMMAND Z80Test)
