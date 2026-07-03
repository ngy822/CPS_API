#include <iostream>
#include <memory>

#include "HardwareCommunication.h"
#include "Logger.h"

bool SIGNAL_DISCONNECTION = false;

#ifdef _WIN32
BOOL WINAPI ConsoleHandler(DWORD signal) {
  if (signal == CTRL_C_EVENT) {
    SIGNAL_DISCONNECTION = true;
  }
  return TRUE;
}

#endif

int main() {
#ifdef _WIN32
  if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
    std::cout << "SetConsoleHandler failed\n";
    return 1;
  }
#endif
  auto logger = std::make_unique<api::laser_tracker::Logger>(true);
  auto broadcast = std::make_shared<api::common::SimpleBroadcastSocket>();
  auto command = std::make_shared<api::common::SimpleTCPClientSocket>();
  auto data = std::make_shared<api::common::SimpleTCPClientSocket>();
  api::laser_tracker::HardwareCommunication hw_comm(broadcast, command, data);
  /*auto error = hw_comm.Connect("192.168.0.168");
  if (error != api::laser_tracker::ErrorType::ErrorType_Success) {
    std::cout << "Connection failed\n";
    return static_cast<int>(error);
  }*/
  std::thread th([&] {
    while (!SIGNAL_DISCONNECTION) {
      // keep running
      api::laser_tracker::RealTimeInfo rt_info;
      if (hw_comm.rt_data_queue_->Pop(rt_info)) {
        if (rt_info.timestamp % 100 == 0) {
          std::cout << rt_info.timestamp << " " << rt_info.az << " "
                    << rt_info.el << " " << rt_info.distance << "\n";
        }
      } else {
        //std::cout << "Failed to receive rt data\n";
      }
    }
  });

  while (!SIGNAL_DISCONNECTION) {
	  // wait for the command
	  if ((GetAsyncKeyState(VK_SHIFT) & 0x01) != 0 && (GetAsyncKeyState(0x4A) & 01) != 0)  {
		  std::cout << "Shift + J pressed\n";
	  }
  }

  hw_comm.Disconnect();
  return 0;
}
