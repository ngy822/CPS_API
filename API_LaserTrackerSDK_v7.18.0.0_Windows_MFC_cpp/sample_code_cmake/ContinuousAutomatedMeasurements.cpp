/******************************************************************************
 * This file is part of the Laser Tracker SDK sample code.
 * Copyright (C) 2026 Automated Precision Inc. All rights reserved.
 * This sample demonstrates how to perform continuous automated measurements
 * with the Laser Tracker SDK.
 * Let's assume we have some predefined targets which we want to measure repeatedly
 * or we know the rough location of the targets in Laser Tracker's frame of
 * reference and we want to go there to measure the target
 *****************************************************************************/
#include <fstream>
#include <thread>
#include <vector>

#include "Common.h"
#include "api/LaserTrackerInterface.h"

const char *DEFAULT_IP_ADDRESS = "192.168.0.168"; /*!< Default IP address for the laser tracker */

/** \brief Helper function to read target locations from a file */
std::vector<api::lt::Vector3<float>> ReadTargetLocationsFromFile(const std::string &filename) {
  std::vector<api::lt::Vector3<float>> target_locations;
  std::ifstream infile(filename);
  if (!infile.is_open()) {
    std::cout << "Failed to open file: " << filename << "\n";
    return target_locations;
  }
  std::string line;
  while (std::getline(infile, line)) {
    // Process each line to extract target locations
    std::istringstream iss(line);
    float x, y, z;
    if (!(iss >> x >> y >> z)) {
      std::cout << "Invalid line format: " << line << "\n";
      continue;
    }
    target_locations.emplace_back(x, y, z);
  }
  infile.close();
  std::cout << "Successfully read " << target_locations.size() << " target locations from file\n";
  return target_locations;
}

/** \brief Function to connect to the laser tracker */
bool Connect(api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data,
             const char *ip_address, api::lt::TaskCallback task_callback_function,
             void *callback_parameter) {
  auto ret = lt->Connect(ip_address, task_callback_function, callback_parameter);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Connection failed: ");
    return false;
  } else {
    std::cout << "Connection started. Wait for callback...\n";
  }
  ret = WaitForTaskCompletion(cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Connection failed: ");
    return false;
  } else {
    std::cout << "Connection succeeded\n";
  }
  return true;
}

/** \brief Function to perform home operation */
bool Home(api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data,
          const api::lt::SmrSize &smr_diameter) {
  auto ret = lt->Home(smr_diameter);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Home operation failed to start: ");
    return false;
  } else {
    std::cout << "Home operation started. Wait for callback...\n";
  }
  ret = WaitForTaskCompletion(cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Home operation failed: ");
    return false;
  } else {
    std::cout << "Home operation succeeded\n";
  }
  return true;
}

/** \brief Function to point to specified cartesian coordinates */
bool PointToCartesian(api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data,
                      float x, float y, float z) {
  api::lt::Vector3<float> position{x, y, z};
  auto ret = lt->PointToCartesian(position);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Point to cartesian operation failed to start: ");
    return false;
  } else {
    std::cout << "Point to cartesian operation started. Wait for callback...\n";
  }
  ret = WaitForTaskCompletion(cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Point to cartesian operation failed: ");
    return false;
  } else {
    std::cout << "Point to cartesian operation succeeded\n";
  }
  return true;
}

/** \brief Function to perform automated measurements at specified target locations */
bool PerformAutomatedMeasurements(api::lt::LaserTrackerInterface *lt,
                                  TaskCallbackFunctionData &cb_data,
                                  const std::vector<api::lt::Vector3<float>> &target_locations) {
  for (size_t i = 0; i < target_locations.size(); ++i) {
    const auto &target = target_locations[i];
    std::cout << "Processing target " << i + 1 << ": (" << target.x << ", " << target.y << ", "
              << target.z << ")\n";
    // Step 1: Point to the target location
    if (!PointToCartesian(lt, cb_data, target.x, target.y, target.z)) {
      std::cout << "Failed to point to target " << i + 1 << "\n";
      continue;
    }
    std::cout << "Successfully pointed to target " << i + 1 << "\n";
    // Step 2: Check if the tracker locked on to the target and is tracking (sometimes it can miss
    // if there is some target movement compared to the time of recording the position)
    api::lt::RealTimeInfo rt_data;
    lt->GetRealTimeData(rt_data);
    if (!rt_data.is_tracking) {
      // If not tracking, perform a spiral search / camera search to find the target
      std::cout << "Tracker is not tracking, we can switch on camera to search for the target\n";
      lt->EnableCameraSearch(true);
      // Wait for some time to let the camera search to find the target and lock on
      bool is_tracking = false;
      // Currently we just wait infinitely. We can add timeout as needed
      while (!is_tracking) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lt->GetRealTimeData(rt_data);
        is_tracking = rt_data.is_tracking;
      }
    }
    // Step 3: At this point, the tracker should be tracking the target, we will wait for
    // measurement_valid flag to be true
    // Currently we just wait infinitely. We can add timeout as needed
    while (!rt_data.is_measurement_valid) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      lt->GetRealTimeData(rt_data);
    }
    std::cout << "Measurement is valid for target " << i + 1 << ", collecting data...\n";
    // Step 4: Collect the data using single point measurement
    api::lt::Vector3<float> measured_point;
    float rms_error;
    constexpr uint32_t averaging_time_ms = 1000;  // we can adjust the averaging time as needed
    auto ret = lt->GetSinglePointMeasurement(averaging_time_ms, measured_point, rms_error);
    if (ret != api::lt::ErrorType::Success) {
      PrintError(lt, ret, "Get single point operation failed: ");
      continue;
    } else {
      std::cout << "Single Point measurement result for target " << i + 1 << ":\n";
      std::cout << "Measured Point: (" << measured_point.x << ", " << measured_point.y << ", "
                << measured_point.z << ")\n";
      std::cout << "RMS Error: " << rms_error << "\n";
    }
  }
  return true;
}

/** \brief Main function for the continuous automated measurements sample application */
int main(int argc, char *argv[]) {
  std::cout << "Laser Tracker Continuous Automated Measurements sample application is running\n";
  api::lt::LaserTrackerInterface lt;
  std::mutex mtx;
  std::condition_variable cv;
  TaskCallbackFunctionData cb_data;
  cb_data.mtx = &mtx;
  cb_data.cv = &cv;
  // Connect to the tracker and home first before performing automated measurements
  if (!Connect(&lt, cb_data, DEFAULT_IP_ADDRESS, nullptr, nullptr)) {
    std::cout << "Failed to connect to the laser tracker.\n";
    return 1;
  }
  // Must home the tracker after connection before performing measurements.
  if (!Home(&lt, cb_data, api::lt::SmrSize::OneHalfInch)) {
    std::cout << "Failed to home the laser tracker.\n";
    return 1;
  }
  while (true) {
    std::cout << "Please enter the filename which contains the target locations in the format of x "
                 "y z (in mm) in each line: Or enter 'quit'/'exit' to exit the application:\n";
    std::string filename;
    std::getline(std::cin, filename);
    if (filename == "quit" || filename == "exit") {
      break;
    }
    std::ifstream infile(filename);
    if (!infile.is_open()) {
      std::cout << "Failed to open file: " << filename << "\n";
      continue;
    }
    infile.close();
    auto target_locations = ReadTargetLocationsFromFile(filename);
    if (target_locations.empty()) {
      std::cout
          << "No valid target locations found in the file. Please check the file and try again.\n";
      continue;
    }
    if (!PerformAutomatedMeasurements(&lt, cb_data, target_locations)) {
      std::cout << "Failed to perform automated measurements.\n";
      continue;
    }
  }
  if (lt.IsDeviceConnected()) {
    lt.Disconnect();
  }
  return 0;
}