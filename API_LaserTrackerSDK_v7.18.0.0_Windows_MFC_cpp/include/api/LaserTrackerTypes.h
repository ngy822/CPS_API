#ifndef LASERTRACKERTYPES_H
#define LASERTRACKERTYPES_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace api {
namespace lt {
/** \defgroup cpp C++
 *  \brief C++ Interface
 *  @{
 */

/** \defgroup constants Constants
 *  \brief Compile time constants used
 *  @{
 */
constexpr auto MAX_ERROR_STRING_LENGTH = 1024;  /*!< Maximum string length to be allocated for the
                                                     error desription. Includes null  terminator. */
constexpr auto MAX_VERSION_STRING_LENGTH = 255; /*!< Maximum string length that firmware version
                                                    number can have. Includes null terminator. */
/** @} */

/** \defgroup error_codes Error Codes
 *  \brief Error codes used in SDK
 *  @{
 */

/**
 * Enumeration for Error reporting
 */
enum class ErrorType : unsigned int {
  Success = 0,               /*!< In case of success. */
  OtherTaskOnGoing,          /*!< In case if trying to invoke a new task when there is a task that
                                           is already running. */
  InvalidCallbackParameter,  /*!< If the callback function parameter set in the connect
                                           function is nullptr. */
  DeviceAlreadyConnected,    /*!< If the device is already connected and trying to connect
                                             again. */
  TaskOperationTimeout,      /*!< In case the assigned task has timedout before completing due
                                           to some error in the device. */
  DataNotUpdating,           /*!< If there is no real time data stream updating due to bad network
                                           communication. */
  OtherMeasurementRunning,   /*!< Trying to launch a new data collection routine when there
                                          is already a data collection routine running. */
  NoMeasurementIsRunning,    /*!< Trying to stop data collection routine when there
                                            is no data collection routine running. */
  FailedToDiscoverIpAddress, /*!< Network communication failed to identify the ip address
                                           of the device. */
  FailedToConnectToFirmware, /*!< Failed to connect to the firmware of the device. */
  FailedToConnectToCamera,   /*!< Connection to camera module failed */
  DeviceNotConnected,        /*!< Trying to invoke operation when device is not connected. */
  FailedToConnectToRealTimeDataPort, /*!< Failed to connect to firmware real time data
                                                       port */
  CommunicationFailed,   /*!< In case when communication failed due to issues with ethernet
                                       communication. */
  InvalidInputParameter, /*!< Input parameter passed to the function is not valid */
  InternalError,         /*!< Internal error occured */
  TaskAborted,           /*!< Task is aborted before completion */
  JoggingLimitReached,   /*!< Gimbal jogging limit reached */
  Unsupported,           /*!< Current operation is not spported */
  LevelAngleOutOfRange,  /*!< Level Sensor readings is out of range */
  MeasurementNotValid,   /*!< Returned when trying to perform measurement and measurement
                                      is not valid. */
  NoSmrAtHomePosition, /*!< There is no SMR at the birdbath position to complete the home operation
                        */
  TrackerNotWarmedUp,  /*!< Cannot perform the operation, tracker is not warmed up. */
  AdmLowIntensity,     /*!< Cannot complete homing operation, ADM intensity is too low. */
  TargetNotTracking,   /*!< Cannot perform the operation because tracker is not tracking the
                                    target. */
  HomingFailure,       /*!< General Homing failure occured. */
  MultiSmrMeasurementFinished, /*!< This is not an error message. It is used as a status to tell
                                  user that multi SMR Data collection is finished */
  FTPManagerCommInitializationFailed, /*!< Could not initialize the ftp protocol.*/
  FTPManagerInvalidIPAddress,         /*!< Invalid IP address for downloading the PRM file. */
  FTPManagerInvalidLocalDirectory,  /*!< Invalid local directory location for downloading the PRM.*/
  FTPManagerInvalidRemoteDirectory, /*!< Invalid remote directory location for downloading the
                                       PRM.*/
  FTPManagerInvalidRemoteSession,   /*!< No valid connection session available.*/
  FTPManagerErrorPerformingFTPCommands,     /*!< Could not perform ftp commands.*/
  FTPManagerInvalidFileName,                /*!< Invalid File name.*/
  FTPManagerRemoteAndLocalFileSizeMismatch, /*!< if local file location size is not equal to remote
                                               file size*/
  FTPManagerCouldNotOpenFileForWriting,     /*!< Specified file could not be opened.*/
  AccessoryPrmFileWrongFormat,              /*!< Accessory PRM file format is wrong. */
  AccessoryPrmFileNotFound,                 /*!< Accessory PRM file is not found. */
  PRMFileIncorrectFormat, /*!< The format of the PRM file was not the expected readable
                             format. */
  PRMFileNotRead,         /*!< The PRM was not read when operation was called. */
  DeviceNotSupported,     /*!< Connected device type is not supported */
  RadiusLimitReached,     /*!< Error returned when spiral radius is bigger than search radius during
                             Spiral search */
  TimedoutNotificationNotReceived, /*!< Timedout before completing the particular task */
  IndexSearchFailed,               /*!< Tracker index search operation failed. */
  TargetIsNotStable,               /*!< Target is not stable during the measurement */
  IvisionWindowNotInitialized,     /*!< Trying to wait for closing before ivision window is open */
  NoProbingDeviceDetected,  /*!< No probing device is detected to perform requested operation */
  NoDataIsCollected,        /*!< No data is collected in the data collection */
  No6DDeviceDetected,       /*!< No 6D Device is detected to perform requested operation */
  VirtualLevelNotPerformed, /*!< Virtual Level is not performed / applied atleast once since
                               connection */
  SmrIsNotLockedOn,         /*!< Trying to do SMR measurement but SMR is not locked on. */
  AccessoryConnected, /*!< Accessory is connected to the tracker. Cannot do requested operation */
  VirtualLevelNotEnabled,  /*!< When the virtual level is not enabled and trying to invoke functions
                              which require virtual level */
  LevelSensorIsNotPresent, /*!< Level sensor is not equipped for the device but trying to do virtual
                              level */
  Iscan3dAppNotFound,      /*!< iScan3d application is not found or installed */
  FailedToStartIscan3dApp, /*!< iScan3d application cannot be started */
  Iscan3dCameraCalibrationFileNotFound, /*!< iScan3D camera calibration file not found */
  Iscan3dCalibrationFileNotFound,       /*!< iScan3D calibration file not found */
  FailedToConnectToIscan3dApp,          /*!< Failed to connect to the iScan3D application */
  FailedToStartIs3dScanning,            /*!< Failed to start iScan3D scanning operation */
  NoScanningDeviceDetected, /*!< No scanning device is detected to perform the operation */
  Iscan3dEncoderIndexSearchNotCompleted, /*!< Cannot perform requested operation because iScan3D
                                            encoder index search is not complete. */
  StylusNotCalibrated, /*!< Current Stylus not calibrated. Cannot use it for measurement. */
  HomingNotPerformed,  /*!< Not homed atleast once since the connection. Home it atleast once to
                           continue with the operation */
  FirmwareTooOld,      /*!< Firmware version is not supported to do the required operation */
};

/** @} */

/** \defgroup enumerations Enumerations
 *  \brief Supporting enumerations used in SDK
 *  @{
 */
/**
 * Enumeration for the angle units
 * It is used only in templates for the PolarVector2 and PolarVector3
 */
enum class AngleUnit { Radian, Degree, Gradian };

/**
 * Enumeration to define different ivision operation modes
 */
enum class IvisionOperationMode : unsigned char {
  Idle,            /*!< No ivision operation mode is running */
  SingleSMR,       /*!< Single SMR ivision operation mode */
  MultiSMR,        /*!< Multi SMR ivision operation mode */
  ManualSMR,       /*!< Manual SMR ivision operation mode */
  TrackCalibration /*!< In-field track calibration */
};

/**
 * Enumeration representing different operation modes of the gimbal
 */
enum class OperationMode : unsigned short {
  Idle,      /*!< Gimbal motors are idle */
  Tracking,  /*!< Tracker is tracking the target */
  Position,  /*!< Tracker is in position mode, it will hold the position and cannot track the target
                in this mode */
  TrackIdle, /*!< Tracker is ready to track the target */
  Searching, /*!< Tracker is searching for the target */
  Internal   /*!< Other internal operation modes. */
};

/**
 * \brief Enumeration for different tasks performed by the device
 * \details All these tasks will call the callback function set during connection
 */
enum class TaskMode : unsigned short {
  Idle = 0,         /*!< Task mode when the device is not performing any task. */
  Connect,          /*!< Task mode returned when the Connect operation is completed. */
  VirtualLevel,     /*!< Task mode returned when VirtualLevel operation is completed. */
  Home,             /*!< Task mode returned when Home operation is completed. */
  PointToCartesian, /*!< Task mode returned when PointToCartesian operation is completed. */
  PointToSpherical, /*!< Task mode returned when PointToSpherical operation is completed. */
  JogTo,            /*!< Task mode returned when JogTo function is completed. */
  JogBy,            /*!< Task mode returned when JogBy function is completed. */
  SpiralSearch,     /*!< Task mode returned when SpiralSearch function is completed. */
  StsJogTo,         /*!< Task mode returned when STS JogTo is completed. */
  StsJogBy,         /*!< Task mode returned when STS JogBy is completed. */
  QvcMeasurement,   /*!< Internal Task. No associated interface function. */
  FrontsightBacksightVerification, /*!< Task mode returned when PeformFrontsightBacksightCheck is
                                      completed */
  Iscan3dScan,                     /*!< Task mode returned when Iscan3d scanning is completed */
  FlipFSBS, /*!< Task mode returned when FlipFrontSightBackSight function is completed. */
  PointToTrackerAndSts, /*!< Task mode returned when PointToTrackerAndSts function is
                                  completed. */
};

/**
 * Defines the SMR diameter used at the birdbath
 */
enum class SmrSize : unsigned char {
  OneHalfInch,    /*!< Diameter of the SMR is 1.5" */
  HalfInch,       /*!< Diameter of the SMR is 0.5" */
  SevenEighthInch /*!< Diameter of the SMR is 7/8" */
};

/**
 * Enumeration to define the identified device model
 */
enum class DeviceModel : unsigned short {
  OldDevice,   /*!< An old device model */
  RadianPro,   /*!< RadianPro device model */
  RadianPlus,  /*!< Radian Plus model */
  iLT,         /*!< iLT model */
  RadianCore2, /*!< Radian Core2 Model */
  RadianPlus2, /*!< Radian Plus2 Model */
};

/**
 * Enumeration to define the identified accessory model
 */
enum class AccessoryModel : unsigned short {
  None,    /*!< No accessory is identified */
  STS,     /*!< STS accessory is identified */
  vProbe2, /*!< vProbe2 accessory is identified */
  WSTS,    /*!< Wireless STS accessory is identified */
  iScan3D, /*!< iScan3D accessory is identified */
};

/**
 * Enumeration to define the probe button signal for the smart button purpose
 */
enum class ProbeButtonSignal : unsigned char {
  Undefined,     /*!< Button is undefined. */
  Up,            /*!< Button is up */
  Down,          /*!< Button is pressed down once */
  DoubleClicked, /*!< Button is double clicked */
  Holding        /*!< Button is held continuously */
};

/**
 * Enumeration to define probe stylus position
 */
enum class ProbeStylusPosition : unsigned char {
  Horizontal, /*!< Stylus is attached to the horizontal position of the probe */
  Vertical    /*!< Stylus is attached to the vertical position of the probe */
};

/**
 * Enumeration to define the stable point collection state
 */
enum class StablePointCollectionState : unsigned char {
  Invalid, /*!< If the data collection mode is not stable point and if the process is not started
              yet. */
  Moving,  /*!< If SMR is moving according to
              StablePointDataCollectionCriteria::distance_error_moving criteria */
  Stable,  /*!< If SMR is stable according to
              StablePointDataCollectionCriteria::distance_error_stable criteria */
  DataCollected /*!< If the SMR is stable and data is collected. Even if the state is DataCollected,
                   there may be error in the data collection as per  MeasurementResult::error_code
                 */
};

/** @} */

/** \defgroup types Functions and Structures
 *  \brief API Defined helper functions and structures used in the SDK
 *  @{
 */

/**
 * \brief Utility structure to represent 2 dimensional cartesian point
 */
template <typename T>
class Vector2 {
 public:
  T x; /*!< x coordinate */
  T y; /*!< y coordinate */

  /**
   * Default constructor
   */
  Vector2() : x(0), y(0) {
  }

  /**
   * Parametrized constructor
   */
  Vector2(T x1, T y1) : x(x1), y(y1) {
  }

  /**
   * Assignment operator
   */
  Vector2 &operator=(const Vector2 &other) {
    x = other.x;
    y = other.y;
    return *this;
  }

  /**
   * Addition operator
   */
  Vector2 operator+(const Vector2 &other) {
    Vector2<T> result;
    result.x = this->x + other.x;
    result.y = this->y + other.y;
    return result;
  }

  /**
   * Addition operator
   */
  Vector2 &operator+=(const Vector2 &other) {
    this->x += other.x;
    this->y += other.y;
    return *this;
  }

  /**
   * Division operator
   */
  Vector2 operator/(const int &divisor) {
    Vector2<T> result;
    if (divisor != 0) {
      result.x = this->x / divisor;
      result.y = this->y / divisor;
    }
    return result;
  }
};

/**
 * Friend function overloading addition operator
 */
template <typename T>
Vector2<T> operator+(const Vector2<T> &lhs, const Vector2<T> &rhs) {
  return Vector2<T>(lhs.x + rhs.x, lhs.y + rhs.y);
}

/**
 * Friend function overloading substraction operator
 */
template <typename T>
Vector2<T> operator-(const Vector2<T> &lhs, const Vector2<T> &rhs) {
  return Vector2<T>(lhs.x - rhs.x, lhs.y - rhs.y);
}

/**
 * \brief Utility structure to represent 3 dimensional cartesian point
 */
template <typename T>
class Vector3 {
 public:
  T x; /*!< x coordinate */
  T y; /*!< y coordinate */
  T z; /*!< z coordinate */

  /**
   * Default constructor
   */
  Vector3() : x(0), y(0), z(0) {
  }

  /**
   * Parametrized constructor
   */
  Vector3(T x1, T y1, T z1) : x(x1), y(y1), z(z1) {
  }

  /**
   * Destructor
   */
  ~Vector3() = default;

  /**
   * Copy constructor
   */
  Vector3(const Vector3 &other) {
    x = other.x;
    y = other.y;
    z = other.z;
  }

  /**
   * Assignment operator
   */
  Vector3 &operator=(const Vector3 &other) {
    if (this != &other) {
      x = other.x;
      y = other.y;
      z = other.z;
    }
    return *this;
  }

  /**
   * Addition operator
   */
  Vector3 operator+(const Vector3 &other) {
    Vector3<T> result;
    result.x = this->x + other.x;
    result.y = this->y + other.y;
    result.z = this->z + other.z;
    return result;
  }

  /**
   * Addition operator
   */
  Vector3 &operator+=(const Vector3 &other) {
    this->x += other.x;
    this->y += other.y;
    this->z += other.z;
    return *this;
  }

  /**
   * Division operator
   */
  Vector3 operator/(const int &divisor) {
    Vector3<T> result;
    if (divisor != 0) {
      result.x = this->x / divisor;
      result.y = this->y / divisor;
      result.z = this->z / divisor;
    }
    return result;
  }
};

/**
 * \brief Constexpr function to support automatic conversion of PolarVector units.
 */
template <typename ScalarType>
constexpr ScalarType AngleUnitToRadianConversionFactor(AngleUnit unit) {
  switch (unit) {
    case AngleUnit::Radian:
      return (ScalarType)1;
    case AngleUnit::Degree:
      return (ScalarType)(3.14159265358979323846 / 180.0);
    case AngleUnit::Gradian:
      return (ScalarType)(3.14159265358979323846 / 200.0);
    default:
      return (ScalarType)0;
  }
}

/**
 * \brief Utility structure to represent gimbal angles
 */
template <typename ScalarType, AngleUnit Unit>
struct PolarVector2 {
  ScalarType az; /*!< AZ angle of the device */
  ScalarType el; /*!< EL angle of the device */

  using value_type = ScalarType;

  /**
   * Default constructor
   */
  PolarVector2() : az(0), el(0) {
  }

  /**
   * Parametrized constructor
   */
  PolarVector2(ScalarType az1, ScalarType el1) : az(az1), el(el1) {
  }

  /**
   * Template copy constructor
   * Automatically convert PolarVector2 into another PolarVector2 of possibly different angle units.
   */
  template <typename OtherScalarType, AngleUnit OtherUnit>
  PolarVector2(const PolarVector2<OtherScalarType, OtherUnit> &other)
      : az(other.az * AngleUnitToRadianConversionFactor<OtherScalarType>(OtherUnit) /
           AngleUnitToRadianConversionFactor<ScalarType>(Unit)),
        el(other.el * AngleUnitToRadianConversionFactor<OtherScalarType>(OtherUnit) /
           AngleUnitToRadianConversionFactor<ScalarType>(Unit)) {
  }
};

// Convenience PolarVector2 type definitions for common ScalarType, AngleUnit combinations
using PolarVector2fd = PolarVector2<float, AngleUnit::Degree>;
using PolarVector2fr = PolarVector2<float, AngleUnit::Radian>;

/**
 * \brief Utility structure to represent gimbal angles and distance
 */
template <typename ScalarType, AngleUnit Unit>
struct PolarVector3 {
  ScalarType az;       /*!< AZ angle of the gimbal */
  ScalarType el;       /*!< EL angle of the gimbal */
  ScalarType distance; /*!< Distance measured by the device (mm) */

  using value_type = ScalarType;

  static constexpr ScalarType Invalid = 0;

  /**
   * Default constructor
   */
  PolarVector3() : az(0), el(0), distance(Invalid) {
  }

  /**
   * Parametrized constructor
   */
  PolarVector3(ScalarType az1, ScalarType el1, ScalarType distance1)
      : az(az1), el(el1), distance(distance1) {
  }

  // Initialize this PolarVector3 with the direction supplied by a PolarVector2, along with the
  // supplied distance. The PolarVector2's angle units are converted into this PolarVector3's
  // units.
  template <typename OtherScalarType, AngleUnit OtherUnit>
  PolarVector3(const PolarVector2<OtherScalarType, OtherUnit> &other, ScalarType distance)
      : az(other.az * AngleUnitToRadianConversionFactor<OtherScalarType>(OtherUnit) /
           AngleUnitToRadianConversionFactor<ScalarType>(Unit)),
        el(other.el * AngleUnitToRadianConversionFactor<OtherScalarType>(OtherUnit) /
           AngleUnitToRadianConversionFactor<ScalarType>(Unit)),
        distance(distance) {
  }

  /**
   * Template copy constructor
   * Automatically convert PolarVector3 into another PolarVector3 of possibly different angle units.
   */
  template <typename OtherScalarType, AngleUnit OtherUnit>
  PolarVector3(const PolarVector3<OtherScalarType, OtherUnit> &other)
      : az(other.az * AngleUnitToRadianConversionFactor<OtherScalarType>(OtherUnit) /
           AngleUnitToRadianConversionFactor<ScalarType>(Unit)),
        el(other.el * AngleUnitToRadianConversionFactor<OtherScalarType>(OtherUnit) /
           AngleUnitToRadianConversionFactor<ScalarType>(Unit)),
        distance(other.distance) {
  }
};

// Convenience PolarVector3 type definitions for common ScalarType, AngleUnit combinations
using PolarVector3fd = PolarVector3<float, AngleUnit::Degree>;
using PolarVector3fr = PolarVector3<float, AngleUnit::Radian>;
using PolarVector3dd = PolarVector3<double, AngleUnit::Degree>;
using PolarVector3dr = PolarVector3<double, AngleUnit::Radian>;

/**
 * \brief Utility structure to represent Rotation angles pitch, roll, yaw
 */
template <typename ScalarType, AngleUnit Unit>
struct RotationAngles {
  ScalarType pitch;
  ScalarType roll;
  ScalarType yaw;

  /**
   * Default constructor
   */
  RotationAngles() : pitch(0), roll(0), yaw(0) {
  }

  /**
   * Parametrized constructor
   */
  RotationAngles(ScalarType pitch1, ScalarType roll1, ScalarType yaw1)
      : pitch(pitch1), roll(roll1), yaw(yaw1) {
  }

  /**
   * Template copy constructor
   * Automatically convert RotationAngles into another RotationAngles of possibly different angle
   * units.
   */
  template <typename OtherScalarType, AngleUnit OtherUnit>
  RotationAngles(const RotationAngles<OtherScalarType, OtherUnit> &other)
      : pitch(other.pitch * AngleUnitToRadianConversionFactor<OtherScalarType>(OtherUnit) /
              AngleUnitToRadianConversionFactor<ScalarType>(Unit)),
        roll(other.roll * AngleUnitToRadianConversionFactor<OtherScalarType>(OtherUnit) /
             AngleUnitToRadianConversionFactor<ScalarType>(Unit)),
        yaw(other.yaw * AngleUnitToRadianConversionFactor<OtherScalarType>(OtherUnit) /
            AngleUnitToRadianConversionFactor<ScalarType>(Unit)) {
  }
};
using RotationAnglesfd = RotationAngles<float, AngleUnit::Degree>;
using RotationAnglesfr = RotationAngles<float, AngleUnit::Radian>;
using RotationAnglesdd = RotationAngles<double, AngleUnit::Degree>;
using RotationAnglesdr = RotationAngles<double, AngleUnit::Radian>;

/**
 * \brief Structure representing device information
 */
struct DeviceInformation {
  char serial_number[MAX_VERSION_STRING_LENGTH];           /*!< Serial number of the device */
  char device_firmware_version[MAX_VERSION_STRING_LENGTH]; /*!< Device firmware version information
                                                            */
  char camera_firmware_version[MAX_VERSION_STRING_LENGTH]; /*!< Camera firmware version information
                                                            */
  DeviceModel device_model;                                /*!< Device model identified */
  AccessoryModel accessory_model;                          /*!< Accessory model identified */
  char accessory_serial_number[MAX_VERSION_STRING_LENGTH]; /*!< Accessory serial number of the
                                                              device */
  /**
   * Default constructor
   */
  DeviceInformation() {
    std::fill(serial_number, serial_number + sizeof(serial_number), 0);
    std::fill(device_firmware_version, device_firmware_version + sizeof(device_firmware_version),
              0);
    std::fill(camera_firmware_version, camera_firmware_version + sizeof(camera_firmware_version),
              0);
    device_model = DeviceModel::OldDevice;
    accessory_model = AccessoryModel::None;
    std::fill(accessory_serial_number, accessory_serial_number + sizeof(accessory_serial_number),
              0);
  }
};

/**
 * \brief Realtime data structure can be used for the display puposes or real time tracking purposes
 */
struct RealTimeInfo {
  unsigned int timestamp; /*!< timestamp of the measurement in ms (resolution is 1ms) */
  float az;               /*!< AZ angle of the 3d point (degree). */
  float el;               /*!< EL angle of the 3d point (degree). */
  float distance;         /*!< distance of the 3d point (mm). */
  float x;                /*!< x coordinate of the 3d point (mm). */
  float y;                /*!< y coordinate of the 3d point (mm). */
  float z;                /*!< z coordinate of the 3d point (mm). */
  bool
      is_measurement_valid; /*!< This field represents if the tracker measurement is valid or not */
  bool is_warmed_up;        /*!< flag to tell if tracker is warmed up */
  bool is_tracking;         /*!< status message to tell whether tracker is tracking */
  OperationMode operation_mode; /*!< Represents the current operation mode of the tracker */
  float body_temperature;       /*!< Body temperature (degree centigrade) */
  int battery_status; /*!< Battery status indicator. Number ranges from 0 to 100 indicating battery
                           percentage.*/
  float level_x; /*!< X channel of Level Sensor. If single level is equipped with the unit, only
                    this channel is valid */
  float level_y; /*!< Y channel of Level Sensor. This is valid only if dual level is available on
                    trackers */
  std::uint64_t global_timestamp; /*!< global timestamp used when synchronizing multiple laser
                                     trackers together. resolution is 1ms */
  /**
   * \brief Nested structure defining STS accessory information
   */
  struct StsInfo {
    bool is_sts_data_available; /*!< Flag to tell whether the STS is detected and data is available
                                 */
    bool is_sts_tracking;       /*!< Flag to tell whether the STS is tracking or not */
    float pitch;                /*!< Pitch angle of the STS device (degree) */
    float roll;                 /*!< Roll angle of the STS device (degree) */
    float yaw;                  /*!< Yaw angle of the STS device (degree) */
    float sts_az;               /*!< AZ angle of the STS gimbal (degree) */
    float sts_el;               /*!< EL angle of the STS gimbal (degree) */
    OperationMode sts_operation_mode; /*!< Operation mode of the STS */
    float rotation_matrix[9]; /*!< Rotation matrix of the STS base with respect to the tracker */
    StsInfo()
        : is_sts_data_available(false),
          is_sts_tracking(false),
          pitch(0.0f),
          roll(0.0f),
          yaw(0.0f),
          sts_az(0.0f),
          sts_el(0.0f),
          sts_operation_mode(OperationMode::Idle) {
      memset(rotation_matrix, 0, sizeof(float) * 9);
      rotation_matrix[0] = rotation_matrix[4] = rotation_matrix[8] = 1.0;
    }
  };
  StsInfo sts_info; /*!< Defines the STS sensor information */

  /**
   * \brief Nested structure defining vProbe2 / iScan3D accessory information
   */
  struct Vprobe2Info {
    bool is_data_available; /*!< Flag to tell whether the vp2 data is available or not */
    bool is_locked_on; /*!< Flag to tell whether the laser beam is locked on to vprobe2 or not */
    float pitch;       /*!< Pitch angle of the vprobe device (degree) */
    float roll;        /*!< Roll angle of the vprobe device (degree) */
    float yaw;         /*!< Yaw angle of the vprobe device (degree) */
    float rotation_matrix[9];    /*!< rotation matrix of the probe with respect to the tracker */
    Vector3<float> tip_position; /*!< Tip position (mm) */
    Vector3<float>
        tip_direction_vector; /*!< Normalized Tip direction vector with respect to tracker. */
    bool encoder_index_search_status; /*!< This is only available for iScan3D. This indicates if
                                         encoder index search status complete or not */
    Vprobe2Info()
        : is_data_available(false),
          is_locked_on(false),
          pitch(0.0f),
          roll(0.0f),
          yaw(0.0f),
          encoder_index_search_status(false) {
      memset(rotation_matrix, 0, sizeof(float) * 9);
      rotation_matrix[0] = rotation_matrix[4] = rotation_matrix[8] = 1.0;
    }
  };
  Vprobe2Info vp2_info; /*!< Defines the vProbe / iScan3D sensor information */

  /**
   * \brief Nested structure defining Diagnostic information
   */
  struct DiagnosticInfo {
    float laser_intensity; /*!< Laser Tube intensity in Volts */
    float adm_intensity;   /*!< ADM Laser Intensity in Volts */
  };
  DiagnosticInfo diagnostic_info; /*!< Defines the tracker diagnostic information */
};

/**
 * \brief Weather station information
 */
struct WeatherStationInfo {
  float air_temperature;       /*!< Air temperature in degree centigrade */
  float air_pressure;          /*!< Air pressure in mmHg */
  float air_humidity;          /*!< Air humidity is a number between 0 to 100 */
  float material_temperature1; /*!< Material temperature in degree centigrade */
  float material_temperature2; /*!< Material temperature in degree centigrade */
  float material_temperature3; /*!< Material temperature in degree centigrade */
  WeatherStationInfo()
      : air_temperature(20),
        air_pressure(760),
        air_humidity(50),
        material_temperature1(0),
        material_temperature2(0),
        material_temperature3(0) {
  }
};

/**
 * \brief Structure representing task results
 */
struct TaskResult {
  TaskMode task;        /*!< Task identifier */
  bool success;         /*!< Indicates whether the task is success or failed */
  ErrorType error_code; /*!< Error code in case of failure. In case of success, this should be
                            ErrorType::Success*/
  unsigned int task_parameters_size; /*!< size of the parameters associated with the task */
  std::unique_ptr<float[]>
      task_parameters; /*!< Task parameters. Not all tasks have the parameters. */

 public:
  /**
   * Default constructor
   */
  TaskResult()
      : task(TaskMode::Idle),
        success(false),
        error_code(ErrorType::Success),
        task_parameters_size(0),
        task_parameters(nullptr) {
  }

  /**
   * Copy constructor
   */
  TaskResult(const TaskResult &taskresult_rhs)
      : task(taskresult_rhs.task),
        success(taskresult_rhs.success),
        error_code(taskresult_rhs.error_code),
        task_parameters_size(taskresult_rhs.task_parameters_size) {
    if (this->task_parameters_size > 0) {
      this->task_parameters.reset(new float[taskresult_rhs.task_parameters_size]);

      for (unsigned int i = 0; i < taskresult_rhs.task_parameters_size; ++i) {
        this->task_parameters[i] = taskresult_rhs.task_parameters[i];
      }
    } else {
      this->task_parameters = nullptr;
    }
  }

  /**
   * Move constructor
   */
  TaskResult(TaskResult &&taskresult_rhs) noexcept : TaskResult() {
    swap(*this, taskresult_rhs);
  }

  /**
   * copy and swap
   */
  TaskResult &operator=(TaskResult taskresult_rhs) {
    swap(*this, taskresult_rhs);
    return *this;
  }

  /**
   * Swap friend function
   */
  friend inline void swap(TaskResult &lhs, TaskResult &rhs) noexcept;
};

/**
 * Friend function to swap TaskResult struct details
 */
inline void swap(TaskResult &lhs, TaskResult &rhs) noexcept {
  using std::swap;

  swap(lhs.task, rhs.task);
  swap(lhs.success, rhs.success);
  swap(lhs.error_code, rhs.error_code);
  swap(lhs.task_parameters_size, rhs.task_parameters_size);
  swap(lhs.task_parameters, rhs.task_parameters);
}

/**
 * \brief Structure representing the measurement result
 */
struct MeasurementResult {
  ErrorType error_code;          /*!< Error code for the data. If this is Success then the measured
                                    point is valid */
  Vector3<float> measured_point; /*!< Measured Point in cartesian coordinates (Unit is mm) */
  float rms_error; /*!< RMS Error of the measurement. Valid only for the stable point data
                      collection. */
  StablePointCollectionState stable_point_collection_state =
      StablePointCollectionState::Invalid; /*!< State of the SMR during stable point
       data collection. */
};

/**
 * \brief Structure representing the measurement result
 */
struct MeasurementResult6D {
  ErrorType error_code;    /*!< Error code for the data. If this is Success then the measured
                              point is valid */
  Vector3<float> position; /*!< Measured Point in cartesian coordinates (Unit is mm) */
  RotationAnglesfd
      orientation; /*!< Orientation of the 6DOF device with respect to tracker (Unit is degree) */
};

/**
 * \brief Structure representing the measurement result for the probe
 */
struct MeasurementResultProbe {
  ErrorType error_code; /*!< Error code for the data. If this is Success then the measured tip data
                           is valid*/
  Vector3<float> tip_position;     /*!< Measured tip position in tracker (Unit is mm) */
  Vector3<float> direction_vector; /*!< Tip Direction vector in tracker */
  float rotation_matrix[9];        /*!< Rotation matrix of the probe orientation in tracker frame */
  float rms_error; /*!< RMS error for the static measurement. This is valid only for single point
                      static measurements*/
};

/**
 * \brief Structure representing the TTL measurement result for the tracker
 */
struct MeasurementResultTTL {
  ErrorType error_code; /*!< Error code for the data. If this is Success then the measured point is
                           valid */
  std::uint32_t timestamp;       /*!< Timestamp of the data from tracker controller */
  Vector3<float> measured_point; /*!< Measured Point in cartesian coordinates */
};

/**
 * \brief Structure representing probe stylus information
 */
struct ProbeStylusDetails {
  ProbeStylusPosition position; /*!< Position to which stylus is attached */
  float diameter;               /*!< Diameter of the tip which is used to measure point (mm) */
  float length;                 /*!< Length of the stylus which is used to measure a point (mm) */
  char id[255];                 /*!< ID of the tip */
};

/**
 * \brief Structure representing Stable Point Data collection criteria.
 * This criteria helps is determing when the target is stable and when the target is moving
 */
struct StablePointDataCollectionCriteria {
  float distance_error_stable; /*!< RMS error in distance buffer to determine that
                                  the target is stable */
  float distance_error_moving; /*!< Minimum peak-to-peak error in distance buffer to determine that
                                  the target is moving */
  unsigned int distance_buffer_size; /*!< Number of elements in the distance buffer */

  /**
   * Default constructor with default parameters
   */
  StablePointDataCollectionCriteria() {
    distance_error_stable = 0.03f;
    distance_error_moving = 0.5f;
    distance_buffer_size = 1000;
  }
};

/**
 * \brief Structure representing Scan Line Data from iScan3D
 */
struct ScanLineData {
  int points_size;                 /*!< Number of points in the array */
  Vector3<float> *points;          /*!< Array of points */
  Vector3<float> direction_vector; /*!< Scanner to object direction vector */

  /**
   * Default constructor
   */
  ScanLineData() : points(nullptr), points_size(0), direction_vector(0.0f, 0.0f, 0.0f) {
  }

  /**
   * Constructor with size as parameter to allocate memory for the points
   */
  ScanLineData(int size)
      : points(new Vector3<float>[size]), points_size(size), direction_vector(0.0f, 0.0f, 0.0f) {
  }

  /**
   * Destructor.
   */
  ~ScanLineData() {
    if (points) {
      delete[] points;
      points = nullptr;
    }
  }

  // No copy or move possible for this structure
  ScanLineData(const ScanLineData &other) = delete;
  ScanLineData(ScanLineData &&other) = delete;
  ScanLineData &operator=(const ScanLineData &other) = delete;
  ScanLineData &operator=(ScanLineData &&other) = delete;
};

/**
 * Structure representing the scan settings
 */
struct ScanSettings {
  float camera_exposure_value = -1.0f; /*!< Exposure value for the camera of the scanner. If the
                                  value is less than 0, Auto Exposure is performed */
};

/**
 * Callback function type for tasks
 * \param task_status Status of the current task
 * \param callback_parameter Parameter that is set when setting the callback function
 */
typedef void (*TaskCallback)(const TaskResult &task_status, void *callback_parameter);

/**
 * Callback function type for dynamic data collection routines and multi SMR routine
 * \param measurement_result Measurement result and error code if any
 * \param callback_parameter Parameter that is set when setting the callback function
 */
typedef void (*MeasurementCallback)(const MeasurementResult &measurement_result,
                                    void *callback_parameter);

/**
 * Callback function type for dynamic data collection routines and multi SMR routine
 * \param measurement_result Measurement result and error code if any
 * \param callback_parameter Parameter that is set when setting the callback function
 */
typedef void (*MeasurementCallback6D)(const MeasurementResult6D &measurement_result,
                                      void *callback_parameter);

/**
 * Callback function type for probing data collection.
 * \param measurement_result Measurement result and error code if any
 * \param callback_parameter Parameter that is set when setting the callback function
 */
typedef void (*MeasurementCallbackProbe)(const MeasurementResultProbe &measurement_result,
                                         void *callback_parameter);

/**
 * Callback function type for TTL data collection
 * \param measurement_result Measurement result with position, timestamp and error code
 * \param callback_parameter Parameter that is set when setting the callback function
 */
typedef void (*MeasurementCallbackTTL)(const MeasurementResultTTL &measurement_result,
                                       void *callback_parameter);

/**
 * Callback function type for notifying stylus change on the probe.
 * \param stylus Gives the basic details for the probe stylus
 * \param callback_parameter Parameter that is set when setting the callback function
 */
typedef void (*StylusChangeCallback)(const ProbeStylusDetails &stylus, void *callback_parameter);

/**
 * Callback function type for getting real time data
 * \param rt_info Current realtime data of the device
 * \param callback_parameter Parameter that is set when setting the callback function
 */
typedef void (*RealTimeDataCallback)(const RealTimeInfo &rt_info, void *callback_parameter);

/**
 * Callback function type for notifying probe button signals.
 * \param signal Gives the button signal specified in the enumeration ProbeButtonSignal
 * \param callback_parameter Parameter that is set when setting the callback function
 */
typedef void (*ProbeSmartButtonCallback)(const ProbeButtonSignal &signal, void *callback_parameter);

/**
 * Callback function type for getting scan line data when scanning in progress
 * \param scan_data Scan line point cloud data
 * \param callback_parameter Parameter that is set when setting the callback function
 */
typedef void (*ScanLineDataCallback)(const ScanLineData &scan_data, void *callback_parameter);

/** @} */
/** @} */
}  // namespace lt
}  // namespace api

#endif  // LASERTRACKERTYPES_H
