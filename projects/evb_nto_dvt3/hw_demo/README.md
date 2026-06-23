# ThreadX Demo

This is a demonstration project based on the ThreadX real-time operating system (RTOS).

## Project Overview

This demo showcases basic ThreadX kernel features, including thread creation, message queue communication, and system timer usage.

### Main Function Behavior

1.  **Hardware Initialization**: The `main` function performs essential hardware initialization, including Pinmap configuration, I2C interface initialization, and setting the CPU frequency to 700MHz.
2.  **Resource Creation**: It creates two message queues for bidirectional communication.
3.  **Thread Interaction**: It creates and starts two test threads:
    *   **Thread 0**: Periodically (every second) sends an incrementing message to Thread 1 and waits for a response. Both sent and received messages are printed via the serial port.
    *   **Thread 1**: Waits for messages from Thread 0. Upon receipt, it sends a predefined response back to Thread 0.
4.  **Main Thread Monitoring**: The `main` thread prints a heartbeat message every 10 seconds to indicate the system is running.

## Build Instructions

This project supports building with the GCC cross-compiler integrated with CMake.

For detailed build steps, dependency requirements, and example commands, please refer to:
[projects/evb_nto/hw_demo/GCC/README.md](GCC/README.md)

## Directory Structure

*   `Src/`: Contains source code, including `main.c`.
*   `Inc/`: Contains header files.
*   `GCC/`: Contains GCC build scripts and configuration files.
