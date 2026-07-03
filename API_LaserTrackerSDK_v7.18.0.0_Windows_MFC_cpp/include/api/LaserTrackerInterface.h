#ifndef LASERTRACKER_INTERFACE_H
#define LASERTRACKER_INTERFACE_H

#ifdef _WIN32
#ifdef LASERTRACKERINTERFACE_EXPORTS
#define LASERTRACKER_API __declspec(dllexport)
#else
#define LASERTRACKER_API __declspec(dllimport)
#endif
#else
#ifdef LASERTRACKERINTERFACE_EXPORTS
#define LASERTRACKER_API __attribute__((visibility("default")))
#else
#define LASERTRACKER_API
#endif
#endif

#include <memory>

#include "api/LaserTrackerTypes.h"

namespace api {
namespace lt {
class LaserTrackerDevice;
class IvisionDeviceAccess;

/** \defgroup cpp C++
 *  \brief C++ Interface
 *  @{
 */

/** \defgroup classes API Laser Tracker Interface class
 *  \brief Main class to interact with the device
 *  @{
 */

/**
 * \brief Interface class for the Laser Tracker Device
 */
class LASERTRACKER_API LaserTrackerInterface {
 public:
  /**
   * \brief default constructor for Laser tracker object
   */
  LaserTrackerInterface();

  /**
   * \brief default destructor
   */
  ~LaserTrackerInterface();

  /**
   * \brief Function to connect to the hardware device.
   * \details This should be the first function to be called in order to setup the connection to
  hardware. On calling this function, it will evaluate the input parameters and return to the
   * caller either Success or an appropriate error code.
   * A task with the id TaskMode::Connect is assigned.
   * Upon the task completion (whether success or failure) a notification callback is issued.
   * With the results of the callback, applications can know the status of the connection.
   *
   * \param[in] ip_address Ethernet ip address for the device.
   * \param[in] callback_function This is the function pointer to get the
   notification of the task completion. For the list of tasks please see, api::lt::TaskMode
   enumeration for more details.
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \param[in] prm_files_path Directory where the prm files for the device has to be stored. If
  nullptr then the current directory is chosen. Default is nullptr
   * \param[in] dll_directory Directory where the platforms directory of Qt library will be placed.
  nullptr then the application directory is chosen. Default is nullptr
   * \return ErrorType::Success In case of success
   * \return ErrorType::DeviceAlreadyConnected In case device already connected and trying
  to connect again
   * \return ErrorType::InvalidCallbackParameter In case callback_function pointer is
  invalid
   */
  ErrorType Connect(const char *ip_address, TaskCallback callback_function,
                    void *callback_parameter = nullptr, const char *prm_files_path = nullptr,
                    const char *dll_directory = nullptr);

  /**
   * \brief Function to connect to the hardware device.
   * \details This should be the first function to be called in order to setup the connection to
  hardware. On calling this function, it will evaluate the input parameters and return to the
   * caller either Success or an appropriate error code.
   * A task with the id TaskMode::Connect is assigned.
   * Upon the task completion (whether success or failure) a notification callback is issued.
   * With the results of the callback, applications can know the status of the connection.
   *
   * \param[in] ip_address Ethernet ip address for the device.
   * \param[in] camera_ip_address Ethernet ip address for the camera submodule.
   * \param[in] callback_function This is the function pointer to get the
   notification of the task completion. For the list of tasks please see, api::lt::TaskMode
   enumeration for more details.
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \param[in] prm_files_path Directory where the prm files for the device has to be stored. If
  nullptr then the current directory is chosen. Default is nullptr
   * \param[in] dll_directory Directory where the platforms directory of Qt library will be placed.
  nullptr then the application directory is chosen. Default is nullptr
   * \return ErrorType::Success In case of success
   * \return ErrorType::DeviceAlreadyConnected In case device already connected and trying
  to connect again
   * \return ErrorType::InvalidCallbackParameter In case callback_function pointer is
  invalid
   */
  ErrorType Connect(const char *ip_address, const char *camera_ip_address,
                    TaskCallback callback_function, void *callback_parameter = nullptr,
                    const char *prm_files_path = nullptr, const char *dll_directory = nullptr);

  /**
   * \brief Function to connect to the hardware device.
   * \details A task with the id TaskMode::Connect is assigned if function returns successfully.
   * Upon the task completion (whether success or failure) a notification callback is issued.
   * With the results of the callback, applications can know the status of the connection.
   *
   * \param[in] model Parameter to specify which device model is used.
   * \param[in] ip_address Ethernet ip address for the device.
   * \param[in] camera_ip_address Ethernet ip address for the camera submodule.
   * \param[in] callback_function This is the function pointer to get the
   notification of the task completion. For the list of tasks please see, api::lt::TaskMode
   enumeration for more details.
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \param[in] prm_files_path Directory where the prm files for the device has to be stored. If
  nullptr then the current directory is chosen. Default is nullptr
   * \param[in] dll_directory Directory where the platforms directory of Qt library will be placed.
  nullptr then the application directory is chosen. Default is nullptr
   * \return ErrorType::Success In case of success
   * \return ErrorType::DeviceAlreadyConnected In case device already connected and trying
  to connect again
   * \return ErrorType::InvalidCallbackParameter In case callback_function pointer is
  invalid
   */
  ErrorType Connect(const DeviceModel &model, const char *ip_address, const char *camera_ip_address,
                    TaskCallback callback_function, void *callback_parameter = nullptr,
                    const char *prm_files_path = nullptr, const char *dll_directory = nullptr);

  /**
   * \brief Function to disconnect the device
   * \details This call disconnects the device and return to the caller.
   * This call is blocking and it will return only after completing disconnection.
   */
  ErrorType Disconnect();

  /**
   * \brief Function to know whether the device is connected or not
   * \return true if device is connected and false if device is disconnected
   */
  bool IsDeviceConnected() const;

  /**
   * \brief Function to command device to perform homing operation
   * \details If the function returns Success, Home is assigned and a callback
   * is sent when the task completes.
   * \param[in] smr_size This will represent the SMR diameter used for homing operation
   * \param[in] adm_offset This offset is used if solid SMR is used during homing operation
   * \return ErrorType::Success In case of success
   * \return ErrorType::DeviceNotConnected If device is not connected
   * \return ErrorType::OtherTaskOnGoing If the device is performing another task
   */
  ErrorType Home(SmrSize smr_size, float adm_offset = 0.0f);

  /**
   * \brief This function call opens up the iVision control window
   *
   * \return ErrorType::Success If window is successfully opened.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::Unsupported For the models such as Radian Plus where the iVision feature is
   * not present
   */
  ErrorType OpenIvisionControlWindow();

  /**
   * \brief Function call to wait for the iVision control window to close down
   *
   * \details This function is useful in console applications or when launching iVision window with
   * a non-UI thread where there are no message loops
   * \return ErrorType::Success If the iVision window is closed
   * \return ErrorType::IvisionWindowNotInitialized If user is trying to wait before iVision window
   * is not opened.
   */
  ErrorType WaitForIvisionWindowClosing();

  /**
   * \brief Function to get images for video stream
   * \details The video frame image contains the pixel in B, G, R byte order
   * \param[in, out] video_frame_image Contains the video frame image with the size
   * width * height* 3. Memory has to be allocated by the user.
   * If this variable to set to nullptr, only the width and height of the image is given out
   * \param[in, out] width Get / Set the width of the video frame image.
   * \param[in, out] height Get / Set the height of the video frame image.
   * \return ErrorType::Success If operation is successful.
   * \return ErrorType::InvalidInputParameter If the input parameters width and height
   * doesn't match video frame width and height
   * \return ErrorType::DeviceNotConnected If device is not connected.
   *
   * \code
   * uint32_t width, height;
   * auto ret = lt->GetVideoFrameImage(nullptr, width, height);
   * if (ret == api::ld::ErrorType::Success) {
   *	uint8_t *video_frame = new uint8_t[width * height * 3];
   *	ret = lt->GetVideoFrameImage(video_frame, width, height);
   *	if (ret == api::ld::ErrorType::Success) {
   *		// do something with the video frame
   *	}
   * }
   * \endcode
   */
  ErrorType GetVideoFrameImage(uint8_t *video_frame_image, uint32_t &width, uint32_t &height);

  /**
   * \brief Function to check if the level sensor is present or not
   * \return true if device is equipped with atleast 1 level sensor.
   * \return false if device is not connected or not equipped with the level sensors
   */
  bool IsLevelSensorPresent() const;

  /**
   * \brief Function to perform virtual level operation for the device
   * \details The device measures level information and calculates a new frame in which Z-Axis is
   * parallel to gravity vector
   * \return ErrorType::Success If set point operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::OtherTaskOnGoing If the device is performing another task.
   */
  ErrorType VirtualLevel();

  /**
   * \brief Get the current virtual level frame information.
   * \param[in,out] matrix Returns a 3x3 rotation matrix in row major order. Memory has to be
   * allocated by the user
   * \return ErrorType::Success If set point operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   */
  ErrorType GetVirtualLevelMatrix(float *matrix);

  /**
   * \brief Set the previous level frame information
   * \param[in] matrix Virtual level matrix of shape 3x3 in row major order can be set
   * \return ErrorType::Success If set point operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   */
  ErrorType SetVirtualLevelMatrix(const float *matrix);

  /**
   * Function to enable or disable virtual level frame transformation to the output points.
   * \return ErrorType::Success If set point operation is successful.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   */
  ErrorType EnableVirtualLevelFrame(bool enable);

  /**
   * \brief Function to point to a specified location
   * \details This function takes in a input point in cartesian coordinates and moves the device to
   * that point. Upon completion, callback is called with the task TaskMode::PointToCartesian.
   * \param[in] position 3D point in cartesian coordinate (all measurements in mm) where the device
   * should move to
   * \param[in] check_gimbal_position If this argument is True, SDK will check whether the gimbal
   * reached it's position before giving a callback. If it is False, SDK will give callback soon
   * after jogging starts. Default is True.
   * \return ErrorType::Success If point to operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::OtherTaskOnGoing If the device is performing another task.
   */
  ErrorType PointToCartesian(const api::lt::Vector3<float> &position,
                             bool check_gimbal_position = true);

  /**
   * \brief Function to point to a specified location
   * \details This function takes in a input point in cartesian coordinates and moves the device to
   * that point. Upon completion, callback is called with the task TaskMode::PointToCartesian.
   * \param[in] position 3D point in spherical coordinate (angles in degree and distance in mm)
   * where the device should move to
   * \param[in] check_gimbal_position If this argument is True, SDK will check whether the gimbal
   * reached it's position before giving a callback. If it is False, SDK will give callback soon
   * after jogging starts. Default is True.
   * \return ErrorType::Success If point to operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::OtherTaskOnGoing If the device is performing another task.
   */
  ErrorType PointToSpherical(
      const api::lt::PolarVector3<float, api::lt::AngleUnit::Degree> &position,
      bool check_gimbal_position = true);

  /**
   * \brief Function to Jog the tracker gimbal to specified az and el location
   * \param[in] az Azimuth angle of the gimbal
   * \param[in] el Elevation angle of the gimbal
   * \param[in] check_gimbal_position If this flag is set to true, callback for the task is received
   * only after finishing the jog, if false, callback for the task is received immediately after it
   * starts jogging. Default is true
   * \return ErrorType::Success If point to operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::OtherTaskOnGoing If the device is performing another task.
   */
  ErrorType JogTo(float az, float el, bool check_gimbal_position = true);

  /**
   * \brief Function to Jog the tracker gimbal relative to current position
   * \param[in] az Azimuth angle of the gimbal
   * \param[in] el Elevation angle of the gimbal
   * \param[in] check_gimbal_position If this flag is set to true, callback for the task is received
   * only after finishing the jog, if false, callback for the task is received immediately after it
   * starts jogging. Default is true
   * \return ErrorType::Success If point to operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::OtherTaskOnGoing If the device is performing another task.
   */
  ErrorType JogBy(float az, float el, bool check_gimbal_position = true);

  /**
   * \brief Function to enable camera based target search
   * \details If enabled camera will start finding the target as soon as it loses the track
   * and keep searching until the target is found or disabled
   * \param[in] enable true if the search has to be always enabled in the background
   * and false to disable the search
   */
  ErrorType EnableCameraSearch(bool enable);

  /**
   * \brief Function to start spiral search to find the target.
   * \details This is a non-blocking function. It returns an callback upon completion of the task
   * \param[in] estimated_distance Approximate distance to the target (in mm).
   * \param[in] estimated_radius Radius of the spiral. The search stops when the spiral goes beyond
   * this radius (in mm).
   * \param[in] timeout Integer to specify the timeout value in ms. After the timeout, search will
   * stop. Default is 40second (40000 ms)
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::InvalidInputParameter If input parameter is invalid.
   * \return ErrorType::OtherTaskOnGoing If the device is performing another task.
   */
  ErrorType SpiralSearch(const float &estimated_distance, const float &estimated_radius,
                         const unsigned int &timeout = 40000);

  /**
   * \brief Function to get the current running task
   * \return Current executing Task from enum TaskMode
   */
  TaskMode GetCurrentTask() const;

  /**
   * \brief Function to get the single point static measurement
   * \param[in] averaging_time Time in ms to collect the data and average to get to the measured
   * point
   * \param[out] measured_point This is the 3D cartesian point measured from the device
   * \param[out] rms_error RMS error of the measured point.
   * \return ErrorType::Success If operation is successfully completed.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::InvalidInputParameter If input parameter is invalid.
   * \return ErrorType::OtherMeasurementRunning If other measurement routine is on-going.
   * \return ErrorType::TaskOperationTimeout If timedout in collecting the data.
   * \note This function is a blocking call. It blocks until the measurement is complete or timeout
   * \note This function will enable the optical switch if it is disabled previously
   */
  ErrorType GetSinglePointMeasurement(uint32_t averaging_time,
                                      api::lt::Vector3<float> &measured_point, float &rms_error);

  /**
   * \brief Function to get dynamic tracker measurement at equal time intervals
   * \param[in] time_period Represents the time period in ms at which tracker measurement is taken.
   * Minimum is 1ms(1kHz Frequency) and maximum is 1000ms (1Hz Frequency)
   * \param[in] callback_function This is the callback function registered in order to get the
   measurement updates
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::InvalidInputParameter If input parameter is invalid.
   * \return ErrorType::OtherMeasurementRunning If other measurement routine is on-going.
   * \note Measurement stops if user commands StopMeasurement() or if there is any error code in the
   MeasurementCallback function.
   * \note This function will enable the optical switch if it is disabled previously
   */
  ErrorType StartTemporalDynamicMeasurement(uint32_t time_period,
                                            MeasurementCallback callback_function,
                                            void *callback_parameter = nullptr);

  /**
   * \brief Function to get dynamic tracker measurement at approximately equal distance
   * \param[in] distance Represents the threshold distance in mm at which tracker measurement is
   taken.
   * This value has to be greater than 0.
   * \param[in] callback_function This is the callback function registered in order to get the
   measurement updates
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::InvalidInputParameter If input parameter is invalid.
   * \return ErrorType::OtherMeasurementRunning If other measurement routine is on-going.
   * \note Measurement stops if user commands StopMeasurement() or if there is any error code in the
   MeasurementCallback function.
   * \note This function will enable the optical switch if it is disabled previously
   */
  ErrorType StartSpatialDynamicMeasurement(float distance, MeasurementCallback callback_function,
                                           void *callback_parameter = nullptr);

  /**
   * \brief Function to measure multiple static SMRs using the camera based search
   * \param[in] averaging_time Time in ms to collect the data and average to get to the measured
   * point. The range is between 100ms to 3000ms
   * \param[in] callback_function This is the callback function registered in order to get the
   measurement updates
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::InvalidInputParameter If input parameter is invalid.
   * \return ErrorType::OtherMeasurementRunning If other measurement routine is on-going.
   * \note Measurement stops if user commands StopMeasurement() or if the error code in the callback
   is ErrorCode::MultiSmrMeasurementFinished
   * \note This function will enable the optical switch if it is disabled previously
   */
  ErrorType StartMultiSmrMeasurement(uint32_t averaging_time, MeasurementCallback callback_function,
                                     void *callback_parameter = nullptr);

  /**
   * \brief Function to start stable point measurement
   * \param[in] averaging_time Time in ms to collect the data and average to get to the measured
   * point
   * \param[in] callback_function This is the callback function registered in order to get the
   measurement updates
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \param[in] criteria Criteria for stable point data collection.
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::InvalidInputParameter If input parameter is invalid.
   * \return ErrorType::OtherMeasurementRunning If other measurement routine is on-going.
   * \note This function will enable the optical switch if it is disabled previously
   */
  ErrorType StartStablePointMeasurement(uint32_t averaging_time,
                                        MeasurementCallback callback_function,
                                        void *callback_parameter = nullptr,
                                        const api::lt::StablePointDataCollectionCriteria &criteria =
                                            api::lt::StablePointDataCollectionCriteria());

  /**
   * \brief Function to stop  the current measurement.
   * \return ErrorType::Success If operation is successfully completed.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::NoMeasurementIsRunning If no measurement is running
   */
  ErrorType StopMeasurement();

  /**
   * \brief Function to abort the current measurement.
   * \return ErrorType::Success If operation is successfully completed.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   */
  ErrorType AbortProcedure();

  /**
   * \brief Function to switch the device motors to idle mode so that it can be moved freely with
   * hands
   */
  ErrorType SwitchToIdle();

  /**
   * \brief Function to switch the device motors to tracking mode so that it tracks or get ready to
   * track the target
   */
  ErrorType SwitchToTrack();

  /**
   * \brief Function to switch the device motors to servo / position mode so that it holds it's
   * position
   */
  ErrorType SwitchToPosition();

  /**
   * \brief Function to switch between the mirror measurement mode and normal SMR mode
   * \param[in] mirror_measurement_mode true will switch to mirror measurement mode and false will
   * switch back to SMR mode.
   * \return ErrorType::Success If operation is successful
   * \return ErrorType::Unsupported If the tracker model is not supported for this operation. Only
   * iLT/iLTx/Radian Plus2/Radian Core2 are supported. Radian Pro/Plus/Core are not supported.
   */
  ErrorType SetMirrorMeasurementMode(bool mirror_measurement_mode);

  /**
   * \brief Function to get device information such as serial number, FW version number
   * \return DeviceInformation If device is connected, it will return with the correct device
   * information
   */
  const DeviceInformation &GetDeviceInformation() const;

  /**
   * \brief Function to get the real time data of the laser radar device
   * \param[out] real_time_data Variable to hold the real time data of the device
   */
  ErrorType GetRealTimeData(api::lt::RealTimeInfo &real_time_data);

  /**
   * \brief Function to get error code details in string for display purposes.
   * This function will provide brief expalanation for the error.
   * \param[in] error_code Variable could be the latest error code returned by SDK for a task.
   * \param[out] error_string Variable should be char pointer and memory should be allocated by the
   * application layer. api::lt::MAX_ERROR_STRING_LENGTH value should be used for allocation.
   */
  void GetErrorMessage(ErrorType error_code, char *error_string) const;

  /**
   * \brief Static Function to get error code details in string for display purposes.
   * This function will provide brief expalanation for the error.
   * \param[in] error_code Variable could be the latest error code returned by SDK for a task.
   * \param[out] error_string Variable should be char pointer and memory should be allocated by the
   * application layer. api::lt::MAX_ERROR_STRING_LENGTH value should be used for allocation.
   */
  static void GetErrorMessageStatic(ErrorType error_code, char *error_string);

  /**
   * \brief Function to get SDK version number
   * \details Memory has to be allocated by the user for the input parameter.
   * It has to be api::lt::MAX_VERSION_STRING_LENGTH.
   */
  void GetSdkVersionNumber(char *version_number) const;

  /**
   * \brief Function to set weather station data
   * \param[in] air_temperature This is the air temperature value user intends to set in degree C
   * \param[in] air_pressure This is the air pressure value user intends to set in mmHg
   * \param[in] air_humidity This is the air humidity value user intends to set.
   * \param[in] use_sensor_readings To tell whether to use the values set by user or use the sensor
   * values.
   * \note Different tracker model behaves differently for this function. To set the weather station
   * values for the **Radian Pro** tracker, user has to unplug the sensor from the tracker and then
   * set it here If the sensor is plugged in, it will always read the sensor value.\n For **Radian
   * Plus/Core**, **iLT/iLTx**, this works based on the use_sensor_readings and the values set.
   *
   * \note Currently it is not possible to set individual sensor values. i.e., if user decides to
   * set the value for air_temperature; air_pressure and air_humidity should also be set.
   */
  ErrorType SetWeatherStationData(const float &air_temperature, const float &air_pressure,
                                  const float &air_humidity, bool use_sensor_readings);

  /**
   * \brief Function to get the weather station data
   */
  ErrorType GetWeatherStationData(api::lt::WeatherStationInfo &info);

  /**
   * \brief Function to enable periodic optical referencing
   *
   */
  ErrorType EnableOpticalReferencing();

  /**
   * \brief Function to disable periodic optical referencing
   * \note When disabled for long time, distance value will drift. Use this function only if advised
   * by API.
   */
  ErrorType DisableOpticalReferencing();

  /**
   * \brief Function to force the optical reference
   */
  ErrorType ForceOpticalReferencing();

  /**
   * \brief Function to set the callback function pointer to get the realtime data at 1000Hz.
   * \return ErrorType::Success If the operation is successful
   * \return ErrorType::DeviceNotConnected If the device is not connected
   * \note This function has to be called after the connection is successful
   */
  ErrorType SetRealTimeCallbackFunction(RealTimeDataCallback callback_function,
                                        void *callback_parameter);

  /**
   * \brief Function to set the target ADM offset.
   * \details This functions sets the ADM offset of the targets. This is useful and user must set
   * this when using Active Target or other Prismatic (Solid) SMRs as targets. This value can be
   * found in the calibration certificate.
   * \param[in] adm_offset ADM Offset value in mm of the target.
   * \return ErrorType::Success If the operation is successful
   * \return ErrorType::DeviceNotConnected If the device is not connected
   */
  ErrorType SetTargetAdmOffset(const float &adm_offset);

  /**
   * \brief Function to flip the Laser tracker gimbal between frontsight and backsight
   * \details The gimbal will be flipped to other side i.e.,
   * $$ az_{target} = 180 \pm az_{current} $$
   * $$ el_{target} = 180 - el_{current} $$
   *
   * \note This function call will only flip the gimbal. It will not perform any measurements
   * If the function returns ErrorType::Success, TaskMode::FlipFSBS is assigned and a
   * callback is sent when the task completes.
   * \return ErrorType::Success If the task is successfully started.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::OtherTaskOnGoing If the device is performing another task.
   */
  ErrorType FlipFrontSightBackSight();

  /************************************************************************************************/
  /*************** TTL and Global Synchronization Measurement Related Functions *******************/
  /**
   * \brief Function to start TTL mode and measurement routine. For this function to start providing
   * measurements,
   * tracker has to be connected to external signal source.
   * \param[in] callback_function This is the callback function registered in order to get the
   * measurement updates
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::OtherMeasurementRunning If other measurement routine is on-going.
   * \return ErrorType::AccessoryConnected If the accessory like vProbe or STS is connected to the
   * tracker.
   * \note TTL mode will not work if an accessory is connected to Laser Tracker
   */
  ErrorType StartTTLMeasurement(MeasurementCallbackTTL callback_function,
                                void *callback_parameter = nullptr);

  /**
   * \brief Function to stop the TTL mode and measurement routine.
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   */
  ErrorType StopTTLMeasurement();

  /**
   * \brief Function to start the global synchronization between multiple laser trackers.
   * \return ErrorType::Success If operation is successfully entered the mode.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::AccessoryConnected If the accessory like vProbe or STS is connected to the
   tracker.
   * \note Even if this function returns Success and in case there is no physical connection between
   trackers or the synchronization box, then global_timestamp will be 0
   */
  ErrorType StartGlobalSynchronizationMode();

  /**
   * \brief Function to stop the global synchronization process
   * \return ErrorType::Success If operation is successfully exited the mode.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   */
  ErrorType StopGlobalSynchronizationMode();
  /************************************************************************************************/

  /************************************************************************************************/
  /*************************** Accessory Related functions ****************************************/

  /**
   * Function to tell whether the laser beam is locked onto accessory or not
   * \return true If the laser beam is locked onto accessory
   * \return false If no accessory is detected at the connection or laser is not locked on to
   * accessory
   */
  bool IsBeamLockedOnAcc() const;

  /**
   * \brief Static function to download the STS PRM file from the STS controller
   * \param[in] sts_ip_address IP address of the STS controller
   * \param[in] prm_files_path Path for the prm files to download
   * \return ErrorType::Success If operation is successfully completed
   */
  static ErrorType DownloadStsPrmFile(const char *sts_ip_address,
                                      const char *prm_files_path = nullptr);

  /**
   * \brief Function to Jog the STS gimbal to specified az and el location.
   * \param[in] az Azimuth angle of the gimbal
   * \param[in] el Elevation angle of the gimbal
   * \param[in] check_gimbal_position If this flag is set to true, callback for the task is received
   * only after finishing the jog, if false, callback for the task is received immediately after it
   * starts jogging. Default is true
   * \return ErrorType::Success If point to operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::OtherTaskOnGoing If the device is performing another task.
   */
  ErrorType StsJogTo(float az, float el, bool check_gimbal_position = true);

  /**
   * \brief Function to Jog the STS gimbal relative to the current location.
   * \param[in] az Azimuth angle of the gimbal
   * \param[in] el Elevation angle of the gimbal
   * \param[in] check_gimbal_position If this flag is set to true, callback for the task is received
   * only after finishing the jog, if false, callback for the task is received immediately after it
   * starts jogging. Default is true
   * \return ErrorType::Success If point to operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::OtherTaskOnGoing If the device is performing another task.
   */
  ErrorType StsJogBy(float az, float el, bool check_gimbal_position = true);

  /**
   * \brief Function to get 6D measurement for the STS.
   * \param[in] averaging_time Time in ms to collect the data and average to get to the measured
   * point
   * \param[out] position This is the 3D cartesian point measured from the device
   * \param[out] orientation This is the orientation of STS with respect to laser tracker
   * \param[out] rms_error RMS error of the measured point
   * \return ErrorType::Success If operation is successfully completed.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::InvalidInputParameter If input parameter is invalid.
   * \return ErrorType::OtherMeasurementRunning If other measurement routine is on-going.
   * \return ErrorType::TaskOperationTimeout If timedout in collecting the data.
   * \note This function is a blocking call. It blocks until the measurement is complete or timeout
   * \note This function will enable the optical switch if it is disabled previously
   */
  ErrorType Get6DSinglePointMeasurement(uint32_t averaging_time, api::lt::Vector3<float> &position,
                                        api::lt::RotationAnglesfd &orientation, float &rms_error);

  /**
   * \brief Function to get 6D measurement for the STS.
   * \param[in] averaging_time Time in ms to collect the data and average to get to the measured
   * point
   * \param[out] position This is the 3D cartesian point measured from the device
   * \param[out] orientation This is the orientation of STS with respect to laser tracker
   * \param[out] matrix This gives the 4x4 homogeneous transformation matrix of the STS with respect
   * to the laser tracker in row major order. Memory of 16 floats has to be allocated by the user.
   * \param[out] rms_error RMS error of the measured point
   * \return ErrorType::Success If operation is successfully completed.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::InvalidInputParameter If input parameter is invalid.
   * \return ErrorType::OtherMeasurementRunning If other measurement routine is on-going.
   * \return ErrorType::TaskOperationTimeout If timedout in collecting the data.
   * \note This function is a blocking call. It blocks until the measurement is complete or timeout
   * \note This function will enable the optical switch if it is disabled previously
   */
  ErrorType Get6DSinglePointMeasurement(uint32_t averaging_time, api::lt::Vector3<float> &position,
                                        api::lt::RotationAnglesfd &orientation, float *matrix,
                                        float &rms_error);

  /**
   * \brief Function to get dynamic tracker measurement at equal time intervals
   * \param[in] time_period Represents the time period in ms at which tracker measurement is taken.
   * Minimum is 10ms(100Hz Frequency) and maximum is 1000ms (1Hz Frequency)
   * \param[in] callback_function This is the callback function registered in order to get the
   measurement updates
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::InvalidInputParameter If input parameter is invalid.
   * \return ErrorType::OtherMeasurementRunning If other measurement routine is on-going.
   * \note Measurement stops if user commands StopMeasurement() or if there is any error code in the
   MeasurementCallback function.
   * \note This function will enable the optical switch if it is disabled previously
   */
  ErrorType Start6DDynamicMeasurement(uint32_t time_period, MeasurementCallback6D callback_function,
                                      void *callback_parameter = nullptr);

  /**
   * \brief Function to know whether STS tracking is affected by the external light source such as
   * Sun.
   * \warning This flag may not give the right judgement at all scenarios.
   * \return true If STS tracking is affected
   * \return false In case if STS tracking is good, if not tracking STS or if
   * no STS detected
   */
  bool IsSTSTrackingAffected() const;

  /**
   * \brief Function to set / register accessory prm file to the tracker
   * \note This do not work with Radian Pro / Radian Plus tracker models without the ethercat box
   * \return ErrorType::Success If operation is successful.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::AccessoryPrmFileNotFound If the file location passed is not correct.
   * \return ErrorType::AccessoryPrmFileWrongFormat If the file has wrong format.
   */
  ErrorType SetAccessoryPrmFile(const char *file_path);

  /**
   * \brief Function to start single point probing measurement. This function prepares the SDK to
   * start collecting the probe data at the button click on the probe device.
   * \param[in] averaging_time Time in ms to collect the data and average to get to the measured
   * point
   * \param[in] callback_function his is the callback function registered in order to get the
   * measurement updates
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::InvalidInputParameter If input parameter is invalid.
   * \return ErrorType::OtherMeasurementRunning If other measurement routine is on-going.
   * \return ErrorType::VirtualLevelNotPerformed If Virtual Level is not performed atleast once
   * after connecting
   */
  ErrorType StartSinglePointProbeMeasurement(uint32_t averaging_time,
                                             MeasurementCallbackProbe callback_function,
                                             void *callback_parameter = nullptr);

  /**
   * \brief Function to start continuous point probing measurement. This function prepares the SDK
   * to start collecting the probe data at the button click and hold on the probe device.
   * \param[in] distance Represents the threshold distance in mm at which tracker measurement is
   * taken.
   * \param[in] callback_function his is the callback function registered in order to get the
   * measurement updates
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::InvalidInputParameter If input parameter is invalid.
   * \return ErrorType::OtherMeasurementRunning If other measurement routine is on-going.
   * \return ErrorType::VirtualLevelNotPerformed If Virtual Level is not performed atleast once
   * after connecting
   */
  ErrorType StartSpatialDynamicProbeMeasurement(float distance,
                                                MeasurementCallbackProbe callback_function,
                                                void *callback_parameter = nullptr);

  /**
   * \brief Function to register to get notification of any stylus change performed on the probing
   * device.
   * \details There are 2 kinds of stylus supported. One is equipped with RFID tag and one without.
   * For the first one as soon as user plugs in the stylus, a notification is sent to the
   * application. For the second case, user has to manually open the selection dialog and select the
   * stylus. In this case after user selects, a notification is sent to the application. In addition
   * to this, whenever user registers to callback the first time, a notification is sent for the
   * purposes of initialization and other scenario, when the laser beam locks on to vProbe, a
   * callback is sent
   * \param[in] callback_function his is the callback function registered in order
   * to get the stylus change updates
   * \param[in] callback_parameter A pointer to data used by the
   * callback. Default is nullptr.
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::NoProbingDeviceDetected If no supported probing device is detected.
   */
  ErrorType RegisterForStylusChangeNotification(StylusChangeCallback callback_function,
                                                void *callback_parameter = nullptr);

  /**
   * \brief Function to register to get notification for the button events on the probing
   * device.
   * \note The smart button signal is received only when the laser beam is not locked on to the
   * probe.
   * \param[in] callback_function This is the callback function registered in order to get the
   * probe button updates
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::NoProbingDeviceDetected If no supported probing device is detected.
   */
  ErrorType RegisterForProbeSmartButtonNotification(ProbeSmartButtonCallback callback_function,
                                                    void *callback_parameter = nullptr);

  /**
   * \brief Function to trigger probe single point measurement. This is equivalent to pressing the
   * physical button on the probe
   * \return ErrorType::Success If operation is successful.
   * \return ErrorType::NoMeasurementIsRunning If no measurement routine is running.
   * \return ErrorType::Unsupported If measurement routine running is different than
   * SinglePointProbeMeasurement.
   */
  ErrorType TriggerProbeMeasurement();

  /**
   * \brief Function to start scanning process with iScan3D
   * \param[in] settings Parameter settings for scanning.
   * \param[in] callback_function This is the function pointer registered to get the scan data
   * points
   * \param[in] callback_parameter A pointer to data used by the callback. Default is nullptr.
   * \return ErrorType::Success If operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::InvalidCallbackParameter If callback function is nullptr
   * \return ErrorType::NoScanningDeviceDetected If iScan3D is not detected.
   * \return ErrorType::Iscan3dEncoderIndexSearchNotCompleted If encoder index search is not
   * completed on the iScan3D.
   */
  ErrorType StartIscan3dScanning(const ScanSettings &settings,
                                 ScanLineDataCallback callback_function,
                                 void *callback_param = nullptr);

  /**
   * \brief Function to stop the scanning process with iScan3D
   */
  ErrorType StopIscan3dScanning();

  /**
   * \brief Function to point the tracker and STS together to a target position. This function will
   * move both tracker and STS
   * \param[in] target_position 3D point in cartesian coordinate (in mm) where the tracker should
   * point to
   * \param[in] sts_angles This is the target azimuth and elevation angles for the STS gimbal.
   * \param[in] check_gimbal_position If this flag is set to true, callback for the task is received
   * only after finishing the jog, if false, callback for the task is received immediately after it
   * starts jogging. Default is true
   * \return ErrorType::Success If point to operation is successfully initiated.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   */
  ErrorType PointToTrackerAndSts(const api::lt::Vector3<float> &target_position,
                                 const api::lt::PolarVector2fd &sts_angles,
                                 bool check_gimbal_position = true);

  /************************************************************************************************/
  /**************************Calibration and verification related functions************************/
  /**
   * \brief Function to open the QVC calibration window and to perform the calibration operation
   * \note This operation is not supported with Radian Pro and Radian Plus/Core
   * \return ErrorType::Success If operation is successful.
   * \return ErrorType::DeviceNotConnected If device is not connected.
   * \return ErrorType::Unsupported Trying to perform this operation with the Radian Pro/Plus/Core.
   */
  ErrorType OpenQvcCalibrationWindow();

  /**
   * \brief Function to perform Front-sight back-sight check.
   * \details Upon successful return of the function TaskMode::FrontsightBacksightVerification task
   * is assigned. It will measure the SMR in both faces and calculates the deviation
   * \pre Tracker must be locked onto to a target already.
   */
  ErrorType PerformFrontsightBacksightCheck();

  /**
   * \brief Function get frontsight backsight check results.
   * \param[out] diff_az Difference in AZ angle between the frontsight and backsight measurements.
   * \param[out] diff_el Difference in EL angle between the frontsight and backsight measurements.
   * \param[out] is_within_tolerance Boolean variable to tell if the difference is within API
   * specification or not.
   * \note The specification is both diff_az < 0.003 and diff_el < 0.003.
   */
  ErrorType GetFrontsightBacksightResults(float &diff_az, float &diff_el,
                                          bool &is_within_tolerance) const;

  /**
   * \brief Function to open the probe calibration window
   * \details This window serves the purpose of Stylus selection in case of Non-RFID stylus.
   * It has ability to add new stylus or edit current stylus, calibrate and verify stylus tip
   * measurements. It also has the ability to perform the In-Field Calibration for the whole device.
   */
  ErrorType OpenProbeCalibrationWindow();

  /**
   * \brief Function to open wireless configuration window to configure connection between tracker
   * and wireless accessory
   * \return ErrorType::Success If operation is successful.
   * \return ErrorType::Unsupported Trying to perform this operation with the Radian Pro.
   */
  ErrorType OpenWirelessConfigurationWindow();

  /**
   * \brief Function to open frontsight backsight check interactive window.
   */
  ErrorType OpenFrontsightBacksightCheckWindow();

  /************************************************************************************************/
  /****************************** Diagnostic Data Dump ********************************************/

  /**
   * \brief Function to dump diagnostic compensated and uncompensated data both to the disk.
   * \note The files will be saved to the current directory as the executable
   * \note This feature is not supported with iLT / RadianCore2
   */
  void DumpDiagnosticData(bool start);

 private:
  std::shared_ptr<LaserTrackerDevice> device_;
  std::unique_ptr<IvisionDeviceAccess> gui_;
};
/** @} */
/** @} */
}  // namespace lt
}  // namespace api

#endif
