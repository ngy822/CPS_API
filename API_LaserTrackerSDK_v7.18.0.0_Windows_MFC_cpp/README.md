# API Laser Tracker SDK – C++

## Overview

This package provides C++ interface for the **API Laser Tracker SDK**, enabling communication with laser tracking hardware directly. It includes sample code, documentation, header files, library files and runtime files.

## Installation

## Windows

1. Use the `bin/`, `include/`, `lib/` and `share/` folders in a location, point `CMAKE_PREFIX_PATH` to that location and sample code should build. Currently it is setup for default folder location.

## Linux

1. Install Laser Tracker SDK Package

Run the following command to install the SDK package. Replace `<version>` with the specific version you are installing.

```bash
sudo dpkg -i LaserTracker-<version>-Linux.deb
```

2. Install additional depencies from apt repository, if not already installed.

```bash
sudo apt update
sudo apt install -y qt6-base-dev
```

## Mac

1. Install Laser Tracker SDK Package
2. Install additional depencies from homebrew repository, if not already installed.

> [!Note]
> Qt6 installation is not required for ARM architectures as the library is built without the dependency on Qt.
> Also, packages with `NoGui` Tag doesn't require Qt6 as it does not have any UI components in it.

## Sample Code Compile Instructions

### Sample Code CMake

```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX="../" ..
cmake --build . --config=Release --target install -j4
```

### Sample Code MFC (Windows Only)

Open Solution in VS2022 and compile the project in x64 Release or Debug

## Documentation

Navigate to share/doc/LaserTracker/html and open `index.html`
