#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <format>
#include <utility>
#include <mutex>

class Logger {
private:
  enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error,
    None,
  };

  std::mutex logMutex;  // Add a mutex for thread safety

public:
  explicit Logger(const std::string& logName = "Logger.log") : logName(logName) {
    pLog.open(logName, std::ios::out); // Open in out mode
  }

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  template <typename... Args>
  void Warn(const std::string& fmt, Args&&... args) noexcept {
    LogMessage(LogLevel::Warn, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Info(const std::string& fmt, Args&&... args) noexcept {
    LogMessage(LogLevel::Info, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Debug(const std::string& fmt, Args&&... args) noexcept {
    LogMessage(LogLevel::Debug, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Error(const std::string& fmt, Args&&... args) noexcept {
    LogMessage(LogLevel::Error, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void Print(const std::string& fmt, Args&&... args) noexcept {
    LogMessage(LogLevel::None, fmt, std::forward<Args>(args)...);
  }

private:
  LogLevel level = LogLevel::Debug;
  std::ofstream pLog;
  std::string logName;

  template <typename... Args>
  void LogMessage(LogLevel logLevel, const std::string& fmt, Args&&... args) noexcept {
    std::scoped_lock lock(logMutex);  // Lock for thread safety

    if (!pLog.is_open()) {
      pLog.open(logName, std::ios::out); // Open in out mode
    }

    std::string logMessage = fmt;
    if (logLevel != LogLevel::None) {
      const char* levelNames[] = {"D", "I", "W", "E", ""};
      logMessage = "[" + std::string(levelNames[static_cast<int>(logLevel)]) + "] " + logMessage;
    }

    pLog << std::vformat(logMessage, std::make_format_args(std::forward<Args>(args)...)) << std::endl;
  }
};
