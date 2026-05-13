cmake_minimum_required(VERSION 3.16)

set(LIB_NAME ex_usb_controller_fun2)

project(${LIB_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

# add_subdirectory(../../src/middlewares/libusb)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# libusb配置
set(LIBUSB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/include)

# 根据平台选择libusb库
if(MSVC)
    # Visual Studio
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LIBUSB_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/VS2022/MS64/static)
    else()
        set(LIBUSB_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/VS2022/MS32/static)
    endif()
    set(LIBUSB_LIB_NAME libusb-1.0.lib)
    set(PLATFORM_LIBS setupapi)
elseif(MINGW)
    # MinGW
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LIBUSB_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/MinGW64/static)
        set(LIBUSB_LIB_NAME libusb-1.0.a)
    else()
        set(LIBUSB_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/libusb/MinGW32/static)
        set(LIBUSB_LIB_NAME libusb-1.0.a)
    endif()
    set(PLATFORM_LIBS setupapi)
else()
    # Linux - 使用系统libusb
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    set(LIBUSB_LIB_NAME ${LIBUSB_LIBRARIES})
    set(PLATFORM_LIBS "")
endif()





add_subdirectory(src)





add_executable(${LIB_NAME} main.cpp)
target_link_libraries(${LIB_NAME} PRIVATE 
    usb_controller_lib
    ${LIBUSB_LIB_NAME}
    ${PLATFORM_LIBS}
)

target_include_directories(${LIB_NAME} PRIVATE
    ${LIBUSB_INCLUDE_DIR}
    ${LIBUSB_INCLUDE_DIRS}
)

target_link_directories(${LIB_NAME} PRIVATE
    ${LIBUSB_LIB_DIR}
)
