cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

project(APILaserTrackerSampleCodeCpp VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")

find_package(LaserTracker REQUIRED)

option(BUILD_ALL_SAMPLES "Build all sample applications" ON)
option(BUILD_BASIC_OPERATIONS "Build Basic Operations sample application" OFF)
option(BUILD_DATA_COLLECTION "Build Data Collection sample application" OFF)
option(BUILD_ACCESSORY_OPERATIONS "Build Accessory Operation" OFF)
option(BUILD_CONTINUOUS_AUTOMATED_MEASUREMENTS "Build Continuous Automated Measurements sample application" OFF)

include(GNUInstallDirs)

if(BUILD_BASIC_OPERATIONS OR BUILD_ALL_SAMPLES)
    add_executable(BasicOperations BasicOperations.cpp)
    target_link_libraries(BasicOperations PRIVATE api::ltinterface)
    install(TARGETS BasicOperations RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(BUILD_DATA_COLLECTION OR BUILD_ALL_SAMPLES)
    add_executable(DataCollection DataCollection.cpp)
    target_link_libraries(DataCollection PRIVATE api::ltinterface)
    install(TARGETS DataCollection RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(BUILD_ACCESSORY_OPERATIONS OR BUILD_ALL_SAMPLES)
    add_executable(AccessoryOperations AccessoryOperations.cpp)
    target_link_libraries(AccessoryOperations PRIVATE api::ltinterface)
    install(TARGETS AccessoryOperations RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(BUILD_CONTINUOUS_AUTOMATED_MEASUREMENTS OR BUILD_ALL_SAMPLES)
    add_executable(ContinuousAutomatedMeasurements ContinuousAutomatedMeasurements.cpp)
    target_link_libraries(ContinuousAutomatedMeasurements PRIVATE api::ltinterface)
    install(TARGETS ContinuousAutomatedMeasurements RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(
    DIRECTORY $<TARGET_FILE_DIR:api::ltinterface>
    DESTINATION .
    FILES_MATCHING PATTERN "*.dll"
)
