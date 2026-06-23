#!/usr/bin/env bash

# Get the directory where the script is located
script_dir=$(dirname "$0")

# Default parameters
output_dir="${script_dir}/../out"
build_type="Debug"
log_level="STATUS"
clean_build="false"
cmake_extra_args=""

# Help information
usage() {
    echo "Usage: $0 [options] -- [extra_cmake_args]
Options:
  -r, --release          Set build type to Release (default is Debug)
  -l, --log-level LEVEL  Set log level (default is STATUS)
  -o, --output DIR       Set output directory (default is <your project>/GCC/out)
  -c, --clean            Enable clean build (delete output directory before build)
  -h, --help             Display this help message
extra_cmake_args:
  Any additional CMake configuration arguments (e.g., -DCONFIG_THREADX_TEST=y -DCONFIG_NEWLIB_LIBC=y)"
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    if [[ "$1" == "--" ]]; then
        shift
        break
    fi
    case "$1" in
        -r|--release)
            build_type="Release"
            shift
            ;;
        -l|--log-level)
            if [[ -n $2 && $2 != --* ]]; then
                log_level="$2"
                shift 2
            else
                echo "Error: --log-level requires a value."
                exit 1
            fi
            ;;
        -o|--output)
            if [[ -n $2 && $2 != --* ]]; then
                output_dir="$2"
                shift 2
            else
                echo "Error: --output requires a value."
                exit 1
            fi
            ;;
        -c|--clean)
            clean_build="true"
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown parameter: $1"
            usage
            exit 1
            ;;
    esac
done

# Capture any extra cmake arguments after the --
cmake_extra_args="$*"

echo "Output dir set to ${output_dir}"
echo "Build type set to ${build_type}"
echo "Log level set to ${log_level}"
echo "Extra CMake args: ${cmake_extra_args}"

# If clean build is enabled, delete the output directory
if [ "$clean_build" = "true" ]; then
    echo "Clean build enabled. Deleting output directory..."
    rm -rf "${output_dir}"
fi

# Check if ccache is available and add to cmake command if present
ccache_flags=""
if command -v ccache &> /dev/null; then
    ccache_flags="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    echo "ccache detected, enabling compiler cache"
fi

# Construct the cmake command
cmake_cmd="cmake -G Ninja -B ${output_dir} -DCMAKE_BUILD_TYPE=${build_type} ${ccache_flags} ${script_dir}/cmake --log-level=${log_level} ${cmake_extra_args}"

echo "${cmake_cmd}"
${cmake_cmd}

# Run ninja in the specified output directory
ninja -C ${output_dir}
