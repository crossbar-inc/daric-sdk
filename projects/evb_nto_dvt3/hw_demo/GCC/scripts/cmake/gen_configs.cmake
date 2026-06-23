cmake_minimum_required(VERSION 3.16)

get_cmake_property(all_variables VARIABLES)

message(DEBUG "All CONFIG_ and HAL_ variables and their values before this script:")
foreach(variable ${all_variables})
    if(variable MATCHES "^CONFIG_" OR variable MATCHES "^HAL_")
        set(var_value ${${variable}})
        if(NOT var_value STREQUAL "n")
            message(DEBUG "${variable}=${var_value}")
            target_compile_definitions(configs INTERFACE "${variable}=${var_value}")
        endif()
    endif()
endforeach()
