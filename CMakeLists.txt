cmake_minimum_required(VERSION 3.10)
project(BAGuitar)

set(CMAKE_CXX_STANDARD 11)

# This toolchain file is based on https://github.com/apmorton/teensy-template/blob/master/Makefile
# and on the Teensy Makefile.

include(CMakeForceCompiler)


# User configurable variables
# (show up in cmake-gui)
#
set(TEENSY_VERSION 36 CACHE STRING "Set to the Teensy version corresponding to your board (30 or 31 allowed)" FORCE)
set(CPU_CORE_SPEED 180000000 CACHE STRING "Set to 24000000, 48000000, 72000000 or 96000000 to set CPU core speed" FORCE)
set(ARDUINOPATH "/Applications/Arduino.app/Contents/Java/" CACHE STRING "Path to Arduino installation" FORCE)


# Derived variables
#
set(CPU cortex-m4)

set(TOOLSPATH "${ARDUINOPATH}hardware/tools/")
set(COMPILERPATH "${TOOLSPATH}arm/bin/")
set(COREPATH "${ARDUINOPATH}hardware/teensy/avr/cores/teensy3/")


# Normal toolchain configuration
#

# this one is important
set(CMAKE_SYSTEM_NAME Generic)

# specify the cross compiler
cmake_force_c_compiler(${COMPILERPATH}arm-none-eabi-gcc GNU)
cmake_force_cxx_compiler(${COMPILERPATH}arm-none-eabi-g++ GNU)

# where is the target environment
set(CMAKE_FIND_ROOT_PATH ${COMPILERPATH})

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


# Additional C and CPP files for Arduino support
#

file(GLOB TEENSY_C_FILES ${COREPATH}*.c)
file(GLOB TEENSY_CPP_FILES ${COREPATH}*.cpp)
set(TEENSY_SOURCES ${TEENSY_C_FILES} ${TEENSY_CPP_FILES})
list(REMOVE_ITEM TEENSY_SOURCES ${COREPATH}main.cpp)

# Teensy 3.0 and 3.1 differentiation
#
message("TEENSY_VERSION ${TEENSY_VERSION}")
if (TEENSY_VERSION EQUAL 30)
    set(CPU_DEFINE __MK20DX128__)
    set(LINKER_FILE ${COREPATH}mk20dx128.ld)
elseif(TEENSY_VERSION EQUAL 36)
    set(CPU_DEFINE __MK66FX1M0__)
    set(LINKER_FILE ${COREPATH}mk66fx1m0.ld)
else()
    set(CPU_DEFINE __MK20DX256__)
    set(LINKER_FILE ${COREPATH}mk20dx256.ld)
endif()

set(TEENSY_DEFINITIONS USB_SERIAL
        LAYOUT_US_ENGLISH
        F_CPU=${CPU_CORE_SPEED}
        ${CPU_DEFINE})


# Macros to wrap add_[executable|library] for seamless Teensy integration
#
function(teensy_add_executable TARGET)
    message("Corepath " ${COREPATH})
    set(ELFTARGET ${TARGET}.elf)
    message("mm ${ELFTARGET} ${ARGN} ${TEENSY_SOURCES}")
    add_executable(${ELFTARGET} ${ARGN} ${TEENSY_SOURCES})

    set_target_properties(${ELFTARGET} PROPERTIES COMPILE_FLAGS "-Wall -g -Os -mcpu=${CPU} -mthumb -nostdlib -MMD -felide-constructors -fno-exceptions -fno-rtti -std=gnu++0x")
    set_target_properties(${ELFTARGET} PROPERTIES COMPILE_DEFINITIONS "${TEENSY_DEFINITIONS} ${INCLUDE_DIRECTORIES} -D __arm__ -DTEENSYDUINO=120")
    set_target_properties(${ELFTARGET} PROPERTIES INCLUDE_DIRECTORIES "${COREPATH}")
    set_target_properties(${ELFTARGET} PROPERTIES LINK_FLAGS "-Os -Wl,--gc-sections,--defsym=__rtc_localtime=0 --specs=nano.specs -mcpu=${CPU} -mthumb -T${LINKER_FILE}")

    add_custom_command(OUTPUT ${TARGET}.hex
            COMMAND ${COMPILERPATH}arm-none-eabi-size ${ELFTARGET}
            COMMAND ${COMPILERPATH}arm-none-eabi-objcopy -O ihex -R .eeprom ${ELFTARGET} ${TARGET}.hex
            COMMAND ${TOOLSPATH}teensy_post_compile -file=${TARGET} -path=${CMAKE_CURRENT_BINARY_DIR} -tools=${TOOLSPATH}
            DEPENDS ${ELFTARGET}
            COMMENT "Creating HEX file for ${ELFTARGET}")

    add_custom_target(hex2 ALL DEPENDS ${TARGET}.hex)
endfunction()

function(teensy_add_library TARGET)
    message("Corepath " ${COREPATH})
    set(ELFTARGET ${TARGET}.elf)
    message("mm ${ELFTARGET} ${ARGN} ${TEENSY_SOURCES}")
    add_library(${ELFTARGET} ${ARGN} ${TEENSY_SOURCES})

    set_target_properties(${ELFTARGET} PROPERTIES COMPILE_FLAGS "-Wall -g -Os -mcpu=${CPU} -mthumb -nostdlib -MMD -felide-constructors -fno-exceptions -fno-rtti -std=gnu++0x")
    set_target_properties(${ELFTARGET} PROPERTIES COMPILE_DEFINITIONS "${TEENSY_DEFINITIONS} ${INCLUDE_DIRECTORIES} -D __arm__ -DTEENSYDUINO=120")
    set_target_properties(${ELFTARGET} PROPERTIES INCLUDE_DIRECTORIES "${COREPATH}")
    set_target_properties(${ELFTARGET} PROPERTIES LINK_FLAGS "-Os -Wl,--gc-sections,--defsym=__rtc_localtime=0 --specs=nano.specs -mcpu=${CPU} -mthumb -T${LINKER_FILE}")

    add_custom_command(OUTPUT ${TARGET}.hex
            COMMAND ${COMPILERPATH}arm-none-eabi-size ${ELFTARGET}
            COMMAND ${COMPILERPATH}arm-none-eabi-objcopy -O ihex -R .eeprom ${ELFTARGET} ${TARGET}.hex
            COMMAND ${TOOLSPATH}teensy_post_compile -file=${TARGET} -path=${CMAKE_CURRENT_BINARY_DIR} -tools=${TOOLSPATH}
            DEPENDS ${ELFTARGET}
            COMMENT "Creating HEX file for ${ELFTARGET}")

    add_custom_target(hex ALL DEPENDS ${TARGET}.hex)
endfunction()

macro(import_arduino_library_base LIB_NAME ROOT_PATH)
    # Check if we can find the library.
    message("import_arduino_library_base '${LIB_NAME}' '${ROOT_PATH}'")
    if(NOT EXISTS ${ROOT_PATH})
        message(FATAL_ERROR "Could not find the Arduino library directory")
    endif(NOT EXISTS ${ROOT_PATH})
    set(LIB_DIR "${ROOT_PATH}/${LIB_NAME}")
    #message(LIB_DIR ${LIB_DIR})
    if(NOT EXISTS "${LIB_DIR}")
        message(FATAL_ERROR "Could not find the directory for library '${LIB_NAME}'")
    endif(NOT EXISTS "${LIB_DIR}")

    # Add it to the include path.
    include_directories("${LIB_DIR}")
    set(INCLUDE_DIRECTORIES "${INCLUDE_DIRECTORIES}-I${LIB_DIR} ")
    message("include_directories " ${LIB_DIR} "  - " ${INCLUDE_DIRECTORIES})

    # Mark source files to be built along with the sketch code.
    file(GLOB SOURCES_CPP ABSOLUTE "${LIB_DIR}" "${LIB_DIR}/*.cpp")
    foreach(SOURCE_CPP ${SOURCES_CPP})
        set(TEENSY_LIB_SOURCES ${TEENSY_LIB_SOURCES} ${SOURCE_CPP})
        message("cpp: ${SOURCE_CPP}")
    endforeach(SOURCE_CPP ${SOURCES_CPP})
    file(GLOB SOURCES_C ABSOLUTE "${LIB_DIR}" "${LIB_DIR}/*.c")
    foreach(SOURCE_C ${SOURCES_C})
        set(TEENSY_LIB_SOURCES ${TEENSY_LIB_SOURCES} ${SOURCE_C})
        message("c: ${SOURCE_C}")
    endforeach(SOURCE_C ${SOURCES_C})

endmacro(import_arduino_library_base)

macro(import_teensy_library LIB_NAME)
    import_arduino_library_base(${LIB_NAME} ${TEENSY_LIB_ROOT})
endmacro(import_teensy_library)

macro(import_arduino_user_library LIB_NAME)
    import_arduino_library_base(${LIB_NAME} ${ARDUINO_USER_LIB_ROOT})
endmacro(import_arduino_user_library)

macro(import_arduino_library LIB_NAME)
    import_arduino_library_base(${LIB_NAME} ${ARDUINO_LIB_ROOT})
endmacro(import_arduino_library)

set(TEENSY_LIB_ROOT "/Applications/Arduino.app/Contents/Java/hardware/teensy/avr/libraries")
set(ARDUINO_LIB_ROOT "/Applications/Arduino.app/Contents/Java/hardware/arduino/avr/libraries")
set(ARDUINO_USER_LIB_ROOT "/Users/xxx/MobileDocs/Documents/Arduino/libraries")

import_arduino_user_library(MIDI_Library/src)

import_teensy_library(Audio)
import_teensy_library(Wire)
import_teensy_library(SPI)
import_teensy_library(SD)
import_arduino_user_library(DmaSpi)
import_arduino_user_library(SerialFlash)

teensy_add_library(BAGuitar
        src/common/AudioDelay.cpp
        src/common/AudioHelpers.cpp
        src/common/ExternalSramManager.cpp
        src/common/ExtMemSlot.cpp
        src/common/IirBiquadFilter.cpp
        src/effects/AudioEffectAnalogDelay.cpp
        src/effects/AudioEffectAnalogDelayFilters.h
        src/effects/BAAudioEffectDelayExternal.cpp
        src/peripherals/BAAudioControlWM8731.cpp
        src/peripherals/BAGpio.cpp
        src/peripherals/BASpiMemory.cpp
        src/AudioEffectAnalogDelay.h
        src/BAAudioControlWM8731.h
        src/BAAudioEffectDelayExternal.h
        src/BAGpio.h
        src/BAGuitar.h
        src/BAHardware.h
        src/BASpiMemory.h
        src/BATypes.h
        src/LibBasicFunctions.h
        src/LibMemoryManagement.h
        src/BAAudioEffectLoopExternal.h
        src/effects/BAAudioEffectLoopExternal.cpp)

include_directories(src)