
# Build System Guide

This project uses the CMake build system and Ninja build tool to manage and build the code. Below are the dependencies and usage instructions for the build system.

## Dependencies

Before starting the build, ensure that the following dependencies are installed on your system:

1. **CMake**: Version 3.16 or higher.
2. **Ninja**: Build tool.
3. **GNU Arm Embedded Toolchain**: Includes tools like `arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, etc.

You can download and install these dependencies from the following links:

- [CMake](https://cmake.org/download/)
- [Ninja](https://ninja-build.org/)
- [GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)

Please ensure that CMake, Ninja and Arm Embedded Toolchain are all added to "PATH"!

Check with:

```bash
cmake --version
ninja --version
arm-none-eabi-gcc --version
```

## Usage

The build script `build.sh` provides a convenient way to build the project. Below are the usage instructions for the script.

`build.sh [options] -- [extra_cmake_args]`

### Command Line Options

- `-r, --release`: Set the build type to `Release` (default is `Debug`).
- `-l, --log-level LEVEL`: Set the log level (default is `STATUS`). Possible values are `ERROR, WARNING, NOTICE, STATUS, VERBOSE, DEBUG, TRACE`.
- `-o, --output DIR`: Set the output directory (default is `<your project>/GCC/out`).
- `-c, --clean`: Enable clean build (delete output directory before build).
- `-h, --help`: Display help message.

### Extra CMake args

Any additional CMake configuration arguments (e.g., -DCONFIG_THREADX_TEST=y)

### Examples

1. Build using default settings:

    ```bash
    ./build.sh
    ```

2. Specify the output directory:

    ```bash
    ./build.sh -o my_output_dir
    ```

3. Use the Release build type:

    ```bash
    ./build.sh -r
    ```

4. Set the log level to `VERBOSE`:

    ```bash
    ./build.sh -l VERBOSE
    ```

5. Combine all options:

    ```bash
    ./build.sh -o my_output_dir -r -l DEBUG
    ```

## CMake Configuration Files

### `CMakeLists.txt`

This file defines the basic configuration of the project, targets, and their properties. Key contents include:

- Setting the C standard version and other compile options.
- Setting the toolchain file path.
- Defining project name and targets.
- Adding necessary libraries and include paths.
- Setting target link options and creating custom targets (e.g., generating `.bin` files).

### Toolchain File

The toolchain file `toolchain-arm-none-eabi.cmake` contains the settings for the cross-compilation toolchain, ensuring the use of `arm-none-eabi-gcc` for compilation.

### Configuration Files

#### Project and board config

Project config: `projects/<board name>/<project name>/GCC/project.cmake`

Board config: `bsp/<board name>/board.cmake`

Set a marco by `set(CONFIG_XXX "<value>")`, you can use the `CONFIG_XXX` marco in both cmake sctipts and c code.