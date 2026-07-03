/**************************************************************************************************
 * This file is part of the Laser Tracker SDK sample code.
 * Copyright (C) 2026 Automated Precision Inc. All rights reserved.
 * Sample code for data collection operations of API Laser Tracker SDK
 **************************************************************************************************/
#include <fstream>

#include "Common.h"
#include "api/LaserTrackerInterface.h"

const char *DEFAULT_IP_ADDRESS = "192.168.0.168"; /*!< Default IP address for the laser tracker */

/** \brief Callback function for handling task completion */
void TaskCallbackFunction(const api::lt::TaskResult &task_status, void *callback_parameter) {
  TaskCallbackFunctionData *data = static_cast<TaskCallbackFunctionData *>(callback_parameter);
  {
    std::lock_guard<std::mutex> lock(*data->mtx);
    data->task_result = task_status;
    data->is_task_completed = true;
  }
  data->cv->notify_one();
}

/** \brief Callback function for handling data collection */
void DataCollectionCallbackFunction(const api::lt::MeasurementResult &measurement_result,
                                    void *callback_parameter) {
  DataCollectionCallbackFunctionData *data =
      static_cast<DataCollectionCallbackFunctionData *>(callback_parameter);
  if (measurement_result.error_code != api::lt::ErrorType::Success) {
    std::cout << "Measurement Error: " << static_cast<unsigned int>(measurement_result.error_code)
              << "\n";
    return;
  }
  std::lock_guard<std::mutex> lock(*data->file_mtx);
  if (data->output_file && data->output_file->is_open()) {
    *(data->output_file) << measurement_result.measured_point.x << ","
                         << measurement_result.measured_point.y << ","
                         << measurement_result.measured_point.z << "\n";
  }
}

/** \brief Callback function for handling TTL data collection */
void TTLDataCollectionCallbackFunction(const api::lt::MeasurementResultTTL &measurement_result,
                                       void *callback_parameter) {
  DataCollectionCallbackFunctionData *data =
      static_cast<DataCollectionCallbackFunctionData *>(callback_parameter);
  if (measurement_result.error_code != api::lt::ErrorType::Success) {
    std::cout << "TTL Measurement Error: "
              << static_cast<unsigned int>(measurement_result.error_code) << "\n";
    return;
  }
  std::lock_guard<std::mutex> lock(*data->file_mtx);
  if (data->output_file && data->output_file->is_open()) {
    *(data->output_file) << measurement_result.timestamp << ","
                         << measurement_result.measured_point.x << ","
                         << measurement_result.measured_point.y << ","
                         << measurement_result.measured_point.z << "\n";
  }
}

/** \brief List of available commands */
static const std::vector<CommandInfo> kCommands = {
    {"list", "", "Lists all available commands"},
    {"connect", "[ip_address]", "Connects to the tracker"},
    {"get_single_point", "<averaging_time_ms>", "Gets a single point measurement"},
    {"start_temporal_dynamic_measurement", "<time_period_ms>",
     "Starts temporal_dynamic measurement"},
    {"start_spatial_dynamic_measurement", "<distance_threshold_mm>",
     "Starts spatial_dynamic measurement"},
    {"stop_measurement", "", "Stops measurement"},
    {"start_ttl_measurement", "", "Starts TTL measurement"},
    {"stop_ttl_measurement", "", "Stops TTL measurement"},
    {"help", "", "Lists all available commands"},
    {"exit", "", "Exits the application"}};

/** \brief Function to perform home operation */
void Home(api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data,
          const api::lt::SmrSize &smr_diameter) {
  auto ret = lt->Home(smr_diameter);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Home operation failed to start: ");
    return;
  } else {
    std::cout << "Home operation started. Wait for callback...\n";
  }
  ret = WaitForTaskCompletion(cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Home operation failed: ");
  } else {
    std::cout << "Home operation succeeded\n";
  }
}

/** \brief Function to connect to the laser tracker */
void Connect(api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data,
             const char *ip_address, api::lt::TaskCallback task_callback_function,
             void *callback_parameter) {
  auto ret = lt->Connect(ip_address, task_callback_function, callback_parameter);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Connection failed: ");
    return;
  } else {
    std::cout << "Connection started. Wait for callback...\n";
  }
  ret = WaitForTaskCompletion(cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Connection failed: ");
  } else {
    std::cout << "Connection succeeded. Homing in progress...\n";
    Home(lt, cb_data, api::lt::SmrSize::OneHalfInch);
  }
}

/** \brief Function to get a single point measurement */
void GetSinglePoint(api::lt::LaserTrackerInterface *lt, uint32_t averaging_time_ms) {
  api::lt::Vector3<float> measured_point;
  float rms_error = 0.0f;
  auto ret = lt->GetSinglePointMeasurement(averaging_time_ms, measured_point, rms_error);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Get single point operation failed: ");
    return;
  } else {
    std::cout << "Single Point measurement result:\n";
    std::cout << "Measured Point: (" << measured_point.x << ", " << measured_point.y << ", "
              << measured_point.z << ")\n";
    std::cout << "RMS Error: " << rms_error << "\n";
  }
}

/** \brief Function to start temporal dynamic measurement */
void StartTemporalDynamicMeasurement(api::lt::LaserTrackerInterface *lt,
                                     DataCollectionCallbackFunctionData &cb_data,
                                     uint32_t time_period_ms) {
  {
    std::lock_guard<std::mutex> lock(*cb_data.file_mtx);
    if (cb_data.output_file) {
      cb_data.output_file->close();
      cb_data.output_file.reset();
    }
    cb_data.output_file = std::make_unique<std::ofstream>("temporal_dynamic_measurement_data.csv");
  }
  auto ret = lt->StartTemporalDynamicMeasurement(time_period_ms, &DataCollectionCallbackFunction,
                                                 (void *)&cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Start temporal dynamic measurement operation failed to start: ");
    return;
  }
  std::cout << "Temporal dynamic measurement started. Receiving data...\n";
  std::cout << "Type stop_measurement command to stop data collection.\n";
}

/** \brief Function to start spatial dynamic measurement */
void StartSpatialDynamicMeasurement(api::lt::LaserTrackerInterface *lt,
                                    DataCollectionCallbackFunctionData &cb_data,
                                    float distance_threshold_mm) {
  {
    std::lock_guard<std::mutex> lock(*cb_data.file_mtx);
    if (cb_data.output_file) {
      cb_data.output_file->close();
      cb_data.output_file.reset();
    }
    cb_data.output_file = std::make_unique<std::ofstream>("spatial_dynamic_measurement_data.csv");
    if (!cb_data.output_file->is_open()) {
      std::cout << "Failed to open output file for spatial dynamic measurement\n";
      return;
    }
  }
  auto ret = lt->StartSpatialDynamicMeasurement(distance_threshold_mm,
                                                &DataCollectionCallbackFunction, (void *)&cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Start spatial dynamic measurement operation failed to start: ");
    return;
  }
  std::cout << "Spatial dynamic measurement started. Receiving data...\n";
  std::cout << "Type stop_measurement command to stop data collection.\n";
}

/** \brief Function to stop measurement */
void StopMeasurement(api::lt::LaserTrackerInterface *lt,
                     DataCollectionCallbackFunctionData &cb_data) {
  auto ret = lt->StopMeasurement();
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Stop measurement operation failed: ");
    return;
  }
  {
    std::lock_guard<std::mutex> lock(*cb_data.file_mtx);
    if (cb_data.output_file) {
      cb_data.output_file->close();
      cb_data.output_file.reset();
    }
  }
  std::cout << "Measurement stopped\n";
}

/** \brief Function to start TTL measurement */
void StartTTLMeasurement(api::lt::LaserTrackerInterface *lt,
                         DataCollectionCallbackFunctionData &cb_data) {
  {
    std::lock_guard<std::mutex> lock(*cb_data.file_mtx);

    if (cb_data.output_file) {
      cb_data.output_file->close();
      cb_data.output_file.reset();
      cb_data.output_file = std::make_unique<std::ofstream>("ttl_measurement_data.csv");
      if (!cb_data.output_file->is_open()) {
        std::cout << "Failed to open output file for TTL measurement\n";
        return;
      }
    }
  }
  auto ret = lt->StartTTLMeasurement(&TTLDataCollectionCallbackFunction, (void *)&cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Start TTL measurement operation failed: ");
    return;
  }
  std::cout << "TTL Measurement started\n";
}

/** \brief Function to stop TTL measurement */
void StopTTLMeasurement(api::lt::LaserTrackerInterface *lt,
                        DataCollectionCallbackFunctionData &cb_data) {
  auto ret = lt->StopTTLMeasurement();
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Stop TTL measurement operation failed: ");
    return;
  }
  {
    std::lock_guard<std::mutex> lock(*cb_data.file_mtx);
    if (cb_data.output_file) {
      cb_data.output_file->close();
      cb_data.output_file.reset();
    }
  }

  std::cout << "TTL Measurement stopped\n";
}

/** \brief Function to execute the given command with provided arguments */
void ExecuteCommand(const std::string &command, const std::vector<std::string> &arguments,
                    api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data,
                    DataCollectionCallbackFunctionData &data_collection_cb_data) {
  if (command == "list") {
    ListAllCommands(kCommands);
  } else if (command == "connect") {
    std::string ip_address = DEFAULT_IP_ADDRESS;
    if (arguments.empty()) {
      std::cout << "Connecting tracker to default IP address 192.168.0.168\n";
    } else {
      ip_address = arguments[0];
      std::cout << "Connecting tracker to IP address " << ip_address << "\n";
    }
    Connect(lt, cb_data, ip_address.c_str(), &TaskCallbackFunction, (void *)&cb_data);
  } else if (command == "get_single_point") {
    if (arguments.size() < 1) {
      std::cout << "Insufficient arguments for get_single_point command\n";
      return;
    }
    uint32_t averaging_time_ms = 1000;
    if (!TryParse<uint32_t>(arguments[0], averaging_time_ms)) {
      std::cout << "Invalid uint32_t value for averaging_time_ms\n";
      return;
    }
    GetSinglePoint(lt, averaging_time_ms);
  } else if (command == "start_temporal_dynamic_measurement") {
    if (arguments.size() < 1) {
      std::cout << "Insufficient arguments for start_temporal_dynamic_measurement command\n";
      return;
    }
    uint32_t time_period_ms = 1;
    if (!TryParse<uint32_t>(arguments[0], time_period_ms)) {
      std::cout << "Invalid uint32_t value for time_period_ms\n";
      return;
    }
    StartTemporalDynamicMeasurement(lt, data_collection_cb_data, time_period_ms);
  } else if (command == "start_spatial_dynamic_measurement") {
    if (arguments.size() < 1) {
      std::cout << "Insufficient arguments for start_spatial_dynamic_measurement command\n";
      return;
    }
    float distance_threshold_mm = 0.0f;
    if (!TryParse<float>(arguments[0], distance_threshold_mm)) {
      std::cout << "Invalid float value for distance_threshold_mm\n";
      return;
    }
    StartSpatialDynamicMeasurement(lt, data_collection_cb_data, distance_threshold_mm);
  } else if (command == "stop_measurement") {
    StopMeasurement(lt, data_collection_cb_data);
  } else if (command == "start_ttl_measurement") {
    StartTTLMeasurement(lt, data_collection_cb_data);
  } else if (command == "stop_ttl_measurement") {
    StopTTLMeasurement(lt, data_collection_cb_data);
  } else if (command == "help" || command == "?") {
    ListAllCommands(kCommands);
  } else {
    std::cout << "Unknown command: " << command << "\n";
  }
}

/** \brief Main function for the data collection sample application */
int main(int argc, char *argv[]) {
  std::cout << "Laser Tracker Data Collection sample application is running\n";
  ListAllCommands(kCommands);
  api::lt::LaserTrackerInterface lt;
  std::mutex mtx;
  std::condition_variable cv;
  TaskCallbackFunctionData cb_data;
  cb_data.mtx = &mtx;
  cb_data.cv = &cv;
  std::mutex file_mtx;
  DataCollectionCallbackFunctionData data_collection_cb_data;
  data_collection_cb_data.file_mtx = &file_mtx;
  std::string line;
  while (true) {
    if (!std::getline(std::cin, line)) {
      break;  // EOF or error
    }
    if (line.empty()) {
      continue;
    }

    if (line == "exit" || line == "quit") {
      break;
    }

    std::vector<char> command(line.begin(), line.end());
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);

    ResetCallbackData(cb_data);
    std::string task;
    std::vector<std::string> arguments;
    ProcessCommand(command, task, arguments);
    ExecuteCommand(task, arguments, &lt, cb_data, data_collection_cb_data);
  }
  if (lt.IsDeviceConnected()) {
    lt.Disconnect();
  }
  return 0;
}