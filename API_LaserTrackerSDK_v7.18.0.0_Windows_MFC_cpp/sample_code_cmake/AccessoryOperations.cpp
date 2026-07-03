/**************************************************************************************************
 * This file is part of the Laser Tracker SDK sample code.
 * Copyright (C) 2026 Automated Precision Inc. All rights reserved.
 * Sample code for accessory operations of API Laser Tracker SDK
 * ************************************************************************************************/

#include <fstream>

#include "Common.h"
#include "api/LaserTrackerInterface.h"

const char *DEFAULT_IP_ADDRESS = "192.168.0.168";

static const std::vector<CommandInfo> kCommands = {
    {"list", "", "Lists all available commands"},
    {"connect", "[ip_address]", "Connects to the tracker"},
    {"virtual_level", "", "Performs virtual level operation"},
    {"single_point_probe_measurement", "<averaging_time_ms>",
     "Performs single point probe measurement with specified averaging time"},
    {"dynamic_probe_measurement", "<distance_threshold_mm>",
     "Performs dynamic probe measurement with specified distance threshold"},
    {"single_point_sts_measurement", "<averaging_time_ms>",
     "Performs single point STS measurement with specified averaging time"},
    {"dynamic_sts_measurement", "<time_period_ms>",
     "Performs dynamic STS measurement with specified time period"},
    {"start_iscan3d_scanning", "", "Starts iScan3D scanning"},
    {"stop_measurement", "", "Stops measurement"},
    {"exit", "", "Exits the application"}};

void TaskCallbackFunction(const api::lt::TaskResult &task_status, void *callback_parameter) {
  TaskCallbackFunctionData *data = static_cast<TaskCallbackFunctionData *>(callback_parameter);
  {
    std::lock_guard<std::mutex> lock(*data->mtx);
    data->task_result = task_status;
    data->is_task_completed = true;
  }
  data->cv->notify_one();
}

void DataCollectionCallbackFunctionProbe(const api::lt::MeasurementResultProbe &measurement_result,
                                         void *callback_parameter) {
  DataCollectionCallbackFunctionData *data =
      static_cast<DataCollectionCallbackFunctionData *>(callback_parameter);
  if (measurement_result.error_code != api::lt::ErrorType::Success) {
    std::cout << "Probe Measurement Error: "
              << static_cast<unsigned int>(measurement_result.error_code) << "\n";
    return;
  }
  {
    std::lock_guard<std::mutex> lock(*data->file_mtx);

    if (data->output_file && data->output_file->is_open()) {
      *(data->output_file) << measurement_result.tip_position.x << ","
                           << measurement_result.tip_position.y << ","
                           << measurement_result.tip_position.z << ","
                           << measurement_result.direction_vector.x << ","
                           << measurement_result.direction_vector.y << ","
                           << measurement_result.direction_vector.z << ","
                           << measurement_result.rms_error << "\n";
    }
  }
}

void DataCollectionCallbackFunctionSTS(const api::lt::MeasurementResult6D &measurement_result,
                                       void *callback_parameter) {
  DataCollectionCallbackFunctionData *data =
      static_cast<DataCollectionCallbackFunctionData *>(callback_parameter);
  if (measurement_result.error_code != api::lt::ErrorType::Success) {
    std::cout << "STS Measurement Error: "
              << static_cast<unsigned int>(measurement_result.error_code) << "\n";
    return;
  }
  {
    std::lock_guard<std::mutex> lock(*data->file_mtx);

    if (data->output_file && data->output_file->is_open()) {
      *(data->output_file) << measurement_result.position.x << "," << measurement_result.position.y
                           << "," << measurement_result.position.z << ","
                           << measurement_result.orientation.pitch << ","
                           << measurement_result.orientation.roll << ","
                           << measurement_result.orientation.yaw << "\n";
    }
  }
}

void Iscan3dScanningCallbackFunction(const api::lt::ScanLineData &measurement_result,
                                     void *callback_parameter) {
  DataCollectionCallbackFunctionData *data =
      static_cast<DataCollectionCallbackFunctionData *>(callback_parameter);
  {
    std::lock_guard<std::mutex> lock(*data->file_mtx);
    if (data->output_file && data->output_file->is_open()) {
      for (int i = 0; i < measurement_result.points_size; ++i) {
        *(data->output_file) << measurement_result.points[i].x << ","
                             << measurement_result.points[i].y << ","
                             << measurement_result.points[i].z << "\n";
      }
    }
  }
}

// Function to perform home operation
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

// Connect to the tracker
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

// Function to perform virtual level operation
void VirtualLevel(api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data) {
  bool apply_persistent_matrix = false;
  std::ifstream fin("virtual_level_matrix.txt");
  float level_matrix[9];
  if (fin.is_open()) {
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        fin >> level_matrix[i * 3 + j];
      }
    }
    fin.close();
    std::cout << "Persistent virtual level matrix found. Do you want to apply it? (y/n):";
    char response = 'n';
    std::cin >> response;
    if (response == 'y' || response == 'Y') {
      apply_persistent_matrix = true;
    }
  }
  if (apply_persistent_matrix) {
    auto ret = lt->SetVirtualLevelMatrix(level_matrix);
    if (ret != api::lt::ErrorType::Success) {
      PrintError(lt, ret, "Apply virtual level matrix operation failed: ");
      return;
    } else {
      std::cout << "Persistent virtual level matrix applied\n";
      return;
    }
  }
  auto ret = lt->VirtualLevel();
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Virtual level operation failed to start: ");
    return;
  } else {
    std::cout << "Virtual level operation started. Wait for callback...\n";
  }
  ret = WaitForTaskCompletion(cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Virtual level operation failed: ");
  } else {
    std::cout << "Virtual level operation succeeded\n";
    // get the level matrix
    float level_matrix[9];
    ret = lt->GetVirtualLevelMatrix(level_matrix);
    if (ret != api::lt::ErrorType::Success) {
      PrintError(lt, ret, "Get virtual level matrix operation failed: ");
      return;
    }
    // save the persistence level matrix
    std::ofstream fout("virtual_level_matrix.txt");
    if (!fout.is_open()) {
      std::cout << "Failed to open file to save virtual level matrix\n";
      return;
    }
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        fout << level_matrix[i * 3 + j] << " ";
      }
      fout << "\n";
    }
    fout.close();
    std::cout << "Virtual level matrix saved to virtual_level_matrix.txt\n";
  }
}

// Function to perform single point probe measurement
void SinglePointProbeMeasurement(api::lt::LaserTrackerInterface *lt,
                                 DataCollectionCallbackFunctionData &cb_data,
                                 uint32_t averaging_time_ms) {
  if (cb_data.output_file) {
    cb_data.output_file->close();
    cb_data.output_file.reset();
  }
  cb_data.output_file = std::make_unique<std::ofstream>("single_point_probe_measurement_data.csv");
  auto ret = lt->StartSinglePointProbeMeasurement(
      averaging_time_ms, &DataCollectionCallbackFunctionProbe, (void *)&cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Single point probe measurement operation failed to start: ");
    return;
  }
  std::cout << "Single point probe measurement started. Receiving data...\n";
}

// Function to perform dynamic probe measurement
void DynamicProbeMeasurement(api::lt::LaserTrackerInterface *lt,
                             DataCollectionCallbackFunctionData &cb_data,
                             float distance_threshold_mm) {
  if (cb_data.output_file) {
    cb_data.output_file->close();
    cb_data.output_file.reset();
  }
  cb_data.output_file = std::make_unique<std::ofstream>("dynamic_probe_measurement_data.csv");
  auto ret = lt->StartSpatialDynamicProbeMeasurement(
      distance_threshold_mm, &DataCollectionCallbackFunctionProbe, (void *)&cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Dynamic probe measurement operation failed to start: ");
    return;
  }
  std::cout << "Dynamic probe measurement started. Receiving data...\n";
}

// Function to perform single point STS measurement
void SinglePointSTSMeasurement(api::lt::LaserTrackerInterface *lt, uint32_t averaging_time_ms) {
  api::lt::Vector3<float> position;
  float rms_error = 0.0f;
  api::lt::RotationAnglesfd orientation;
  auto ret = lt->Get6DSinglePointMeasurement(averaging_time_ms, position, orientation, rms_error);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Single point STS measurement operation failed to start: ");
    return;
  }
  std::cout << "Single Point STS Measurement result:\n";
  std::cout << "Measured Position: (" << position.x << ", " << position.y << ", " << position.z
            << ")\n";
  std::cout << "Measured Orientation (Pitch, Roll, Yaw in degrees): (" << orientation.pitch << ", "
            << orientation.roll << ", " << orientation.yaw << ")\n";
  std::cout << "RMS Error: " << rms_error << "\n";
}

// Function to perform dynamic STS measurement
void DynamicSTSMeasurement(api::lt::LaserTrackerInterface *lt,
                           DataCollectionCallbackFunctionData &cb_data, uint32_t time_period_ms) {
  if (cb_data.output_file) {
    cb_data.output_file->close();
    cb_data.output_file.reset();
  }
  cb_data.output_file = std::make_unique<std::ofstream>("dynamic_sts_measurement_data.csv");
  auto ret = lt->Start6DDynamicMeasurement(time_period_ms, &DataCollectionCallbackFunctionSTS,
                                           (void *)&cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Dynamic STS measurement operation failed to start: ");
    return;
  }
  std::cout << "Dynamic STS measurement started. Receiving data...\n";
}

// Function to perform iScan3D scanning
void StartIscan3dScanning(api::lt::LaserTrackerInterface *lt,
                          DataCollectionCallbackFunctionData &data_collection_cb_data) {
  if (data_collection_cb_data.output_file) {
    data_collection_cb_data.output_file->close();
    data_collection_cb_data.output_file.reset();
  }
  data_collection_cb_data.output_file =
      std::make_unique<std::ofstream>("iscan3d_scanning_data.csv");
  auto ret = lt->StartIscan3dScanning(api::lt::ScanSettings(), &Iscan3dScanningCallbackFunction,
                                      (void *)&data_collection_cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Start iScan3D scanning operation failed to start: ");
    return;
  }
  std::cout << "iScan3D scanning started. Wait for callback...\n";
}

// Stop measurement
void StopMeasurement(api::lt::LaserTrackerInterface *lt,
                     DataCollectionCallbackFunctionData &cb_data) {
  auto ret = lt->StopMeasurement();
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Stop measurement operation failed: ");
    return;
  }
  if (cb_data.output_file) {
    cb_data.output_file->close();
    cb_data.output_file.reset();
  }
  std::cout << "Measurement stopped\n";
}

// Execute the given command with provided arguments
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
  } else if (command == "virtual_level") {
    VirtualLevel(lt, cb_data);
  } else if (command == "single_point_probe_measurement") {
    if (arguments.size() < 1) {
      std::cout << "Insufficient arguments for single_point_probe_measurement command\n";
      return;
    }
    uint32_t averaging_time_ms = 1000;
    if (!TryParse<uint32_t>(arguments[0], averaging_time_ms)) {
      std::cout << "Invalid uint32_t value for averaging_time_ms\n";
      return;
    }
    SinglePointProbeMeasurement(lt, data_collection_cb_data, averaging_time_ms);
  } else if (command == "dynamic_probe_measurement") {
    if (arguments.size() < 1) {
      std::cout << "Insufficient arguments for dynamic_probe_measurement command\n";
      return;
    }
    float distance_threshold_mm = 1.0f;
    if (!TryParse<float>(arguments[0], distance_threshold_mm)) {
      std::cout << "Invalid float value for distance_threshold_mm\n";
      return;
    }
    DynamicProbeMeasurement(lt, data_collection_cb_data, distance_threshold_mm);
  } else if (command == "single_point_sts_measurement") {
    if (arguments.size() < 1) {
      std::cout << "Insufficient arguments for single_point_sts_measurement command\n";
      return;
    }
    uint32_t averaging_time_ms = 1000;
    if (!TryParse<uint32_t>(arguments[0], averaging_time_ms)) {
      std::cout << "Invalid uint32_t value for averaging_time_ms\n";
      return;
    }
    SinglePointSTSMeasurement(lt, averaging_time_ms);
  } else if (command == "dynamic_sts_measurement") {
    if (arguments.size() < 1) {
      std::cout << "Insufficient arguments for dynamic_sts_measurement command\n";
      return;
    }
    uint32_t time_period_ms = 100;
    if (!TryParse<uint32_t>(arguments[0], time_period_ms)) {
      std::cout << "Invalid uint32_t value for time_period_ms\n";
      return;
    }
    DynamicSTSMeasurement(lt, data_collection_cb_data, time_period_ms);
  } else if (command == "start_iscan3d_scanning") {
    StartIscan3dScanning(lt, data_collection_cb_data);
  } else if (command == "stop_measurement") {
    StopMeasurement(lt, data_collection_cb_data);
  } else if (command == "help" || command == "?") {
    ListAllCommands(kCommands);
  } else {
    std::cout << "Unknown command: " << command << "\n";
  }
}

// main function
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