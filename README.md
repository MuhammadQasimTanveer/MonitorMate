# MonitorMate - System Resource Monitor

A comprehensive Windows system resource monitoring application built in C++.

## Features

- Real-time CPU and memory monitoring
- Process management and termination
- Graphical performance visualization
- Continuous monitoring mode
- Data logging capabilities

## Prerequisites

- Windows 10/11
- Visual Studio 2019/2022 with C++ development tools
- Docker Desktop (for containerized builds)

## Building and Running

### Traditional Build

1. Open `MonitorMate.sln` in Visual Studio
2. Select your configuration (Debug/Release) and platform (x64)
3. Build the solution
4. Run the generated executable

## Usage

The application provides a console-based menu interface with the following options:

1. CPU Performance Metrics
2. Memory Usage Statistics
3. View Running Processes
4. View All Processes
5. Sort Processes by CPU/Memory
6. System Overview Dashboard
7. Kill Process by PID
8. View Killed Processes History
9. Continuous Real-time Monitoring
10. Set Refresh Interval
11. Export Data to Log File
12. Auto-refresh Dashboard
13. Exit

## Docker Notes

- The current Dockerfile uses Windows Server Core as the base image
- For full build automation, Visual Studio Build Tools would need to be installed in the container
- The application is Windows-specific and uses Windows APIs
