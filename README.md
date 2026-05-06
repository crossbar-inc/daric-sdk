## Daric SDK

Daric SDK is a comprehensive software development kit providing drivers, middleware, and tools for firmware development on Daric hardware platforms.

### Directory Structure

#### [bsp/](bsp/)
- **Description**: Board Support Packages (BSPs). Contains configurations for specific hardware platforms (e.g., `Daric-EVB-NTO`) and common component drivers.

#### [documents/](documents/)
- **Description**: Project-related documentation, including technical docs in Sphinx format.

#### [drivers/](drivers/)
- **Description**: Peripheral drivers for Daric, including Hardware Abstraction Layer (HAL) and low-level drivers (e.g., UART, SPI, I2C, USB, Flash, etc.).
- **Key Subdirectories**:
    - `Src/sce`: Low-level driver implementations for the Secure Compute Engine (SCE).

#### [middleware/](middleware/)
- **Description**: Middleware and third-party libraries.
- **Key Components**:
    - `crossbar/crypto`: SCE algorithm library, including implementations for ECDSA, EdDSA, Paillier, ZKP, and more.
    - `third_party`: Third-party libraries including `mbedtls` (cryptography), `threadx` (RTOS), `cmsis`, and a custom `libc`.

#### [projects/](projects/)
- **Description**: Specific application projects, such as the firmware project for the `EVB-NTO` development board.

#### [utilities/](utilities/)
- **Description**: Auxiliary tools and utilities, including flashing tools (`flash`), PC-side tools (`pc_tools`), and SVD files for debugging.


## Hardware Resources

For Daric hardware-related resources, please refer to the hardware repository:

[https://github.com/crossbar-inc/daric-hardware](https://github.com/crossbar-inc/daric-hardware)

## Build Firmware

### Install toolchain

- Download and install the ARM GNU Toolchain (version 14.3):

    https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads

    Make sure `arm-none-eabi-gcc` is in `$PATH` search path.

- Other tools:

    ```bash
    brew install cmake
    brew install ninja
    ```

## Coding Style

This project follows the [BARR-C:2018](https://barrgroup.com/embedded-systems/books/barr-c-coding-standard) Embedded C Coding Standard.

To ensure consistent code style across the project, a `.clang-format` configuration file is provided in the root directory. Developers are encouraged to format their code using the `clang-format` tool before submitting:

```bash
clang-format -i <file_path>
```

## Licensing

The Daric Open Source Project is licensed under the [Apache License, Version 2.0](LICENSE).

This project also includes various third-party components and drivers, which are subject to their own respective licenses. For a complete list of third-party attributions and license information, please refer to [LICENSE-3RD-PARTY.txt](LICENSE-3RD-PARTY.txt).
