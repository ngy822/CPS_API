/******************************************************************************
 * This file is part of the Laser Tracker SDK sample code.
 * Copyright (C) 2026 Automated Precision Inc. All rights reserved.
 * Common data structures and helper functions for Laser Tracker SDK samples
 *****************************************************************************/

#ifndef COMMON_DATA_STRUCTURES_H
#define COMMON_DATA_STRUCTURES_H

#include <charconv>
#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "api/LaserTrackerInterface.h"

/**
 * \brief Data structure to hold the callback data for task completion notification
 */
struct TaskCallbackFunctionData {
  std::mutex *mtx = nullptr;
  std::condition_variable *cv = nullptr;
  api::lt::TaskResult task_result;
  bool is_task_completed = false;
};

/**
 * \brief Data structure to hold the command information for the sample applications
 */
struct CommandInfo {
  const char *name;
  const char *params;
  const char *description;
  const char *category;
};

/**
 * \brief Data structure to hold the callback data for measurement data collection
 */
struct DataCollectionCallbackFunctionData {
  std::mutex *file_mtx = nullptr;
  std::unique_ptr<std::ofstream> output_file;
};

/**
 * \brief Helper function to wait for task completion using condition variable and return the error
 * code
 */
inline api::lt::ErrorType WaitForTaskCompletion(const TaskCallbackFunctionData &cb_data) {
  std::unique_lock<std::mutex> lock(*cb_data.mtx);
  cb_data.cv->wait(lock, [&cb_data]() { return cb_data.is_task_completed; });
  return cb_data.task_result.error_code;
}

/**
 * \brief Helper function to reset the callback data before starting a new task
 */
inline void ResetCallbackData(TaskCallbackFunctionData &data) {
  std::lock_guard<std::mutex> lock(*data.mtx);
  data.is_task_completed = false;
  data.task_result = api::lt::TaskResult();
}

/**
 * \brief Helper function to list all supported commands in this sample application
 */
inline void ListAllCommands(const std::vector<CommandInfo> &commands) {
  std::cout << "Supported commands:\n\n";

  for (const auto &cmd : commands) {
    std::cout << "  " << std::left << std::setw(16) << cmd.name << std::left << std::setw(40)
              << cmd.params << cmd.description << "\n";
  }
}

/**
 * \brief Helper function to process the user input command and separate the command and its
 * arguments
 */
inline void ProcessCommand(const std::vector<char> &command, std::string &task,
                           std::vector<std::string> &arguments) {
  arguments.clear();
  task.clear();
  std::string input(command.begin(), command.end());

  std::istringstream iss(input);
  if (!(iss >> task)) {
    return;  // empty command
  }
  std::string arg;
  while (iss >> arg) {
    arguments.push_back(arg);
  }
}

/**
 * \brief Helper function to print error message for the error codes returned by the API functions
 */
inline void PrintError(api::lt::LaserTrackerInterface *lt, const api::lt::ErrorType &err,
                       const char *prefix) {
  if (!lt) {
    std::cout << prefix << "Unknown error (null interface)\n";
    return;
  }
  char msg[api::lt::MAX_ERROR_STRING_LENGTH];
  lt->GetErrorMessage(err, msg);
  std::cout << prefix << msg << "\n";
}

/**
 * \brief Helper function template to parse string to numeric types with error checking
 */
template <typename T>
inline bool TryParse(const std::string &str, T &out) {
  static_assert(std::is_arithmetic_v<T> || std::is_same_v<T, bool>,
                "TryParse supports arithmetic and bool types only");

  const char *begin = str.data();
  const char *end = str.data() + str.size();

  if constexpr (std::is_same_v<T, bool>) {
    if (str == "true" || str == "1") {
      out = true;
      return true;
    }
    if (str == "false" || str == "0") {
      out = false;
      return true;
    }
    return false;
  } else if constexpr (std::is_integral_v<T>) {
    auto result = std::from_chars(begin, end, out);
    return result.ec == std::errc{} && result.ptr == end;
  } else if constexpr (std::is_floating_point_v<T>) {
    auto result = std::from_chars(begin, end, out);
    return result.ec == std::errc{} && result.ptr == end;
  }
}

#endif  // COMMON_DATA_STRUCTURES_H