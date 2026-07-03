/******************************************************************************
 * This file is part of the Laser Tracker SDK sample code.
 * Copyright (C) 2026 Automated Precision Inc. All rights reserved.
 * Sample code for basic operations of API Laser Tracker SDK in C++
 * This sample demonstrates how to connect to the laser tracker device and
 * perform various operations such as homing, virtual leveling, jogging,
 * pointing, and retrieving real-time data. It also shows how to handle
 * asynchronous task completion using callbacks and condition variables.
 *****************************************************************************/
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

/** List of available commands */
static const std::vector<CommandInfo> kCommands = {
    {"list", "", "Lists all available commands"},
    {"connect", "[ip_address]", "Connects to the tracker"},
    {"home", "[smr_diameter: 0|1|2]", "Performs home operation"},
    {"virtual_level", "", "Performs virtual level operation"},
    {"jog_to", "<az_angle> <el_angle>", "Jog to absolute angles"},
    {"jog_by", "<az_angle> <el_angle>", "Jog by relative angles"},
    {"point_to_cartesian", "<x> <y> <z>", "Point to specified cartesian coordinates"},
    {"point_to_spherical", "<az_angle> <el_angle> <distance>",
     "Point to specified spherical coordinates"},
    {"get_rt_data", "", "Gets one frame of real-time data"},
    {"enable_camera_search", "<0|1>", "Enable or disable camera autolock"},
    {"spiral_search", "<estimated distance> <estimated search radius> [timeout]",
     "spiral search to find the target"},
}; 

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
    std::cout << "Connection succeeded\n";
  }
}

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

/** \brief Function to perform virtual level operation */
void VirtualLevel(api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data) {
  bool apply_persistent_matrix = false;
  std::ifstream fin("virtual_level_matrix.txt");
  float level_matrix[9];
  if (fin.is_open()) {
    bool read_ok = true;
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        if (!(fin >> level_matrix[i * 3 + j])) {
          read_ok = false;
          break;
        }
      }
      if (!read_ok)
        break;
    }
    fin.close();
    if (!read_ok) {
      std::cout << "Failed to read virtual level matrix from file\n";
    } else {
      std::cout << "Persistent virtual level matrix found. Do you want to apply it? (y/n):";
      char response = 'n';
      std::cin >> response;
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(),
                      '\n');  // consume leftover newline
      if (response == 'y' || response == 'Y') {
        apply_persistent_matrix = true;
      }
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

/** \brief Function to jog to specified angles */
void JogTo(api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data, float az_angle,
           float el_angle) {
  auto ret = lt->JogTo(az_angle, el_angle);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Jog to operation failed to start: ");
    return;
  } else {
    std::cout << "Jog to operation started. Wait for callback...\n";
  }
  ret = WaitForTaskCompletion(cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Jog to operation failed: ");
  } else {
    std::cout << "Jog to operation succeeded\n";
  }
}

/** \brief Function to jog by specified angles */
void JogBy(api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data, float az_angle,
           float el_angle) {
  auto ret = lt->JogBy(az_angle, el_angle);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Jog by operation failed to start: ");
    return;
  } else {
    std::cout << "Jog by operation started. Wait for callback...\n";
  }
  ret = WaitForTaskCompletion(cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Jog by operation failed: ");
  } else {
    std::cout << "Jog by operation succeeded\n";
  }
}

/** \brief Function to point to specified cartesian coordinates */
void PointToCartesian(api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data,
                      float x, float y, float z) {
  api::lt::Vector3<float> position{x, y, z};
  auto ret = lt->PointToCartesian(position);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Point to cartesian operation failed to start: ");
    return;
  } else {
    std::cout << "Point to cartesian operation started. Wait for callback...\n";
  }
  ret = WaitForTaskCompletion(cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Point to cartesian operation failed: ");
  } else {
    std::cout << "Point to cartesian operation succeeded\n";
  }
}

/** \brief Function to point to specified spherical coordinates */
void PointToSpherical(api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data,
                      float distance, float az_angle, float el_angle) {
  api::lt::PolarVector3<float, api::lt::AngleUnit::Degree> position{az_angle, el_angle, distance};
  auto ret = lt->PointToSpherical(position);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Point to spherical operation failed to start: ");
    return;
  } else {
    std::cout << "Point to spherical operation started. Wait for callback...\n";
  }
  ret = WaitForTaskCompletion(cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Point to spherical operation failed: ");
  } else {
    std::cout << "Point to spherical operation succeeded\n";
  }
}

/** \brief Function to enable or disable camera search */
void EnableCameraSearch(api::lt::LaserTrackerInterface *lt, bool enable) {
  auto ret = lt->EnableCameraSearch(enable);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Enable camera search operation failed: ");
    return;
  } else {
    if (enable) {
      std::cout << "Camera search enabled\n";
    } else {
      std::cout << "Camera search disabled\n";
    }
  }
}

/** \brief Function to print real-time data */
void PrintRealtimeData(const api::lt::RealTimeInfo &rt_data) {
  constexpr int labelWidth = 25;
  constexpr int valueWidth = 12;

  std::cout << std::boolalpha;                      // print bools as true/false
  std::cout << std::fixed << std::setprecision(3);  // set decimal precision
  auto print_line = [&](const char *label, auto value) {
    std::cout << std::left << std::setw(labelWidth) << label << std::right << std::setw(valueWidth)
              << value << "\n";
  };
  print_line("Timestamp (ms):", rt_data.timestamp);
  print_line("AZ (deg):", rt_data.az);
  print_line("EL (deg):", rt_data.el);
  print_line("Distance (mm):", rt_data.distance);
  print_line("X (mm):", rt_data.x);
  print_line("Y (mm):", rt_data.y);
  print_line("Z (mm):", rt_data.z);
  print_line("Measurement valid:", rt_data.is_measurement_valid);
  print_line("Tracker warmed up:", rt_data.is_warmed_up);
  print_line("Tracker tracking:", rt_data.is_tracking);
}

/** \brief Function to get one frame of real-time data */
void GetRealTimeData(api::lt::LaserTrackerInterface *lt) {
  api::lt::RealTimeInfo rt_data;
  auto ret = lt->GetRealTimeData(rt_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Get real-time data operation failed: ");
    return;
  } else {
    std::cout << "Real-time data received:\n";
    PrintRealtimeData(rt_data);
  }
}

/** \brief Function to perform spiral search to find the target */
void SpiralSearch(api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data,
                  float estimated_distance, float estimated_search_radius, unsigned int timeout) {
  auto ret = lt->SpiralSearch(estimated_distance, estimated_search_radius, timeout);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Spiral search operation failed to start: ");
    return;
  } else {
    std::cout << "Spiral search operation started. Wait for callback...\n";
  }
  ret = WaitForTaskCompletion(cb_data);
  if (ret != api::lt::ErrorType::Success) {
    PrintError(lt, ret, "Spiral search operation failed: ");
  } else {
    std::cout << "Spiral search operation succeeded\n";
  }
}

/** \brief Function to execute the given command with provided arguments */
void ExecuteCommand(const std::string &command, const std::vector<std::string> &arguments,
                    api::lt::LaserTrackerInterface *lt, TaskCallbackFunctionData &cb_data) {
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
  } else if (command == "home") {
    api::lt::SmrSize smr_diameter = api::lt::SmrSize::OneHalfInch;
    if (arguments.empty()) {
      std::cout << "Performing home operation with default SMR diameter 1.5 inch\n";
    } else {
      unsigned long val = 0;
      if (!TryParse<unsigned long>(arguments[0], val) || val > 2) {
        std::cout << "Invalid smr_diameter value. Use 0, 1, or 2.\n";
        return;
      }
      smr_diameter = static_cast<api::lt::SmrSize>(val);
    }
    Home(lt, cb_data, smr_diameter);
  } else if (command == "virtual_level") {
    VirtualLevel(lt, cb_data);
  } else if (command == "jog_to") {
    if (arguments.size() < 2) {
      std::cout << "Insufficient arguments for jog_to command\n";
      return;
    }
    // Usage example:
    float az, el;
    if (!TryParse<float>(arguments[0], az) || !TryParse<float>(arguments[1], el)) {
      std::cout << "Invalid numeric arguments for jog_to command\n";
      return;
    }
    JogTo(lt, cb_data, az, el);
  } else if (command == "jog_by") {
    if (arguments.size() < 2) {
      std::cout << "Insufficient arguments for jog_by command\n";
      return;
    }
    float az, el;
    if (!TryParse<float>(arguments[0], az) || !TryParse<float>(arguments[1], el)) {
      std::cout << "Invalid numeric arguments for jog_by command\n";
      return;
    }
    JogBy(lt, cb_data, az, el);
  } else if (command == "point_to_cartesian") {
    if (arguments.size() < 3) {
      std::cout << "Insufficient arguments for point_to_cartesian command\n";
      return;
    }
    float x, y, z;
    if (!TryParse<float>(arguments[0], x) || !TryParse<float>(arguments[1], y) ||
        !TryParse<float>(arguments[2], z)) {
      std::cout << "Invalid numeric arguments for point_to_cartesian command\n";
      return;
    }
    PointToCartesian(lt, cb_data, x, y, z);
  } else if (command == "point_to_spherical") {
    if (arguments.size() < 3) {
      std::cout << "Insufficient arguments for point_to_spherical command\n";
      return;
    }
    float az, el, distance;
    if (!TryParse<float>(arguments[0], az) || !TryParse<float>(arguments[1], el) ||
        !TryParse<float>(arguments[2], distance)) {
      std::cout << "Invalid numeric arguments for point_to_spherical command\n";
      return;
    }
    PointToSpherical(lt, cb_data, az, el, distance);
  } else if (command == "get_rt_data") {
    GetRealTimeData(lt);
  } else if (command == "enable_camera_search") {
    if (arguments.size() < 1) {
      std::cout << "Insufficient arguments for enable_camera_search command\n";
      return;
    }
    int enable = 0;
    if (!TryParse<int>(arguments[0], enable)) {
      std::cout << "Invalid integer value for enable_camera_search command\n";
      return;
    }
    EnableCameraSearch(lt, (bool)enable);
  } else if (command == "spiral_search") {
    if (arguments.size() < 2) {
      std::cout << "Insufficient arguments for spiral_search command\n";
      return;
    }
    float estimated_distance;
    float estimated_search_radius;
    if (!TryParse<float>(arguments[0], estimated_distance) ||
        !TryParse<float>(arguments[1], estimated_search_radius)) {
      std::cout << "Invalid numeric arguments for spiral_search command\n";
      return;
    }
    unsigned int timeout = 40000;  // default timeout
    if (arguments.size() >= 3) {
      if (!TryParse<unsigned int>(arguments[2], timeout)) {
        std::cout << "Invalid timeout value for spiral_search command\n";
        return;
      }
    }
    SpiralSearch(lt, cb_data, estimated_distance, estimated_search_radius, timeout);
  } else if (command == "help" || command == "?") {
    ListAllCommands(kCommands);
  } else {
    std::cout << "Unknown command: " << command << "\n";
  }
}

/** \brief Main function for the laser tracker sample application */
int main(int argc, char *argv[]) {
  std::cout << "Laser Tracker sample application is running\n";
  ListAllCommands(kCommands);
  api::lt::LaserTrackerInterface lt;
  std::mutex mtx;
  std::condition_variable cv;
  TaskCallbackFunctionData cb_data;
  cb_data.mtx = &mtx;
  cb_data.cv = &cv;
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
    ExecuteCommand(task, arguments, &lt, cb_data);
  }
  if (lt.IsDeviceConnected()) {
    lt.Disconnect();
  }
  return 0;
}