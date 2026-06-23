
add_library(NFC STATIC)

target_link_libraries(NFC PRIVATE configs)

target_link_libraries(NFC PUBLIC driver)
target_link_libraries(NFC PRIVATE threadx)
target_link_libraries(NFC PRIVATE filex)
if(CONFIG_SUPPORT_NFC)
    target_link_libraries(NFC PRIVATE guix)
endif()
if(CONFIG_TESTSUITE_HAL_NFC)
    target_link_libraries(NFC PRIVATE usercase)
endif()

# 获取所有子目录
file(GLOB_RECURSE ALL_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# 从头文件列表中提取包含目录
foreach(HEADER_FILE ${ALL_HEADERS})
    get_filename_component(HEADER_DIR ${HEADER_FILE} DIRECTORY)
    list(APPEND INCLUDE_DIRS ${HEADER_DIR})
endforeach()

# 去除重复的目录
list(REMOVE_DUPLICATES INCLUDE_DIRS)
target_include_directories(NFC PUBLIC ${INCLUDE_DIRS})
# target_include_directories(NFC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/port/include)
target_include_directories(NFC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/port)
# target_include_directories(NFC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/examples)
# target_include_directories(NFC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/examples/soft_alg)

# 查找当前目录下的所有.c文件
file(GLOB_RECURSE SRC_FILES "*.c")

target_sources(NFC PRIVATE ${SRC_FILES})
