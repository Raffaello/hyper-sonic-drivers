cmake_minimum_required (VERSION 3.24)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_DEBUG_POSTFIX d)

if(USE_SPDLOG)
    find_package(spdlog REQUIRED CONFIG)
    add_compile_definitions(HAS_SPDLOG=1)
    set(LIB_SPDLOG spdlog::spdlog)
endif()

if(USE_SDL3)
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
    add_compile_definitions(HAS_SDL3=1)
    set(LIB_SDL SDL3::SDL3)
elseif(USE_SDL2)
    find_package(SDL2 CONFIG REQUIRED)
    add_compile_definitions(HAS_SDL2=1)
    set(LIB_SDL SDL2::SDL2)
else()
endif()

if(USE_MT32)
    find_package(MT32Emu 2.7 CONFIG REQUIRED)
    set(LIB_MT32EMU MT32Emu::mt32emu)
    add_compile_definitions(HAS_MT32_EMU=1)
else()
    set(LIB_MT32EMU)
endif()

configure_file("version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/version.h")


########################### Static Lib #######################################
set(LIB_NAME hyper-sonic-drivers-static)
set(LIB_CONF ${LIB_NAME}-config)
set(LIB_VER ${LIB_CONF}-version)
include(CMakePackageConfigHelpers)

add_library(${LIB_NAME} STATIC)
set_target_properties(${LIB_NAME} PROPERTIES
     POSITION_INDEPENDENT_CODE ON
     COMPILE_WARNING_AS_ERROR ON
)

target_compile_options(${LIB_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:
            -Wall
            # -Wextra
            -Wpedantic
        >
)

if(GENERATE_CODE_COVERAGE)
    target_compile_options(${LIB_NAME}
        PUBLIC
         $<$<CXX_COMPILER_ID:GNU>:
            $<$<CONFIG:DEBUG>: --coverage>
        >
    )

    target_link_options(${LIB_NAME} PUBLIC
        $<$<CXX_COMPILER_ID:GNU>:
            $<$<CONFIG:DEBUG>: --coverage>
         >
    )

    target_link_libraries(${LIB_NAME} PUBLIC
        $<$<CXX_COMPILER_ID:GNU>:
            $<$<CONFIG:DEBUG>: gcov>
         >
    )
endif()

## TODO: this is ok, but some includes files i don't want to be "public",
##       so it looks like it must be built manually..
# try defining PUBLIC_HEADER than do an install target (it is at line 31)
install(DIRECTORY
    src/
    DESTINATION static/include
    FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.h"
)

install(DIRECTORY
    test/
    DESTINATION static/include/test
    FILES_MATCHING
        PATTERN "*Mock.hpp"
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${LIB_VER}.cmake
  VERSION ${CMAKE_PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
  )

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${LIB_VER}.cmake
    DESTINATION static/cmake
)

target_include_directories(${LIB_NAME}
    PUBLIC
        # $<BUILD_INTERFACE:$<TARGET_FILE_DIR:${LIB_NAME}>>
        # $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:static/include>
        $<INSTALL_INTERFACE:static/include/test>
#PRIVATE
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_sources(${LIB_NAME} PRIVATE
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/IMixer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/mixer/Channel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/mixer/MixerBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/rtaudio/Mixer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/Renderer.cpp
    $<$<BOOL:${USE_SDL2}>:${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/sdl2/Mixer.cpp>
    $<$<BOOL:${USE_SDL3}>:${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/sdl3/Mixer.cpp>
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/MIDI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/midi/MIDITrack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/PCMSound.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/streams/PCMStream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/streams/EmulatedStream.cpp
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/converters/IRateConverter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/converters/bitsConverter.cpp
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/scummvm/Timestamp.cpp
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/audio/opl/banks/OP2Bank.cpp
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/devices/IDevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/devices/Opl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/devices/Adlib.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/devices/SbPro.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/devices/SbPro2.cpp
    $<$<BOOL:${USE_MT32}>:${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/devices/MT32.cpp>
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/PCMDriver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/IAudioDriver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/MIDDriver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/midi/IMidiDriver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/midi/IMidiChannel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/midi/IMidiChannelVoice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/westwood/ADLDriver.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/midi/scummvm/MidiDriver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/midi/scummvm/AdLibInstrument.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/midi/scummvm/AdLibChannel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/midi/scummvm/AdLibPercussionChannel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/midi/scummvm/MidiDriver_ADLIB.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/midi/opl/OplDriver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/midi/opl/OplChannel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/midi/opl/OplVoice.cpp

    $<$<BOOL:${USE_MT32}>:${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/midi/mt32/MT32Driver.cpp>

    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/opl/OplWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/drivers/opl/IMFDriver.cpp
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/File.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/IFFFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/westwood/ADLFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/MIDFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/dmx/MUSFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/dmx/OP2File.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/miles/XMIFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/miles/AILDriverFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/RIFFFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/IPCMFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/WAVFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/VOCFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/files/IMFFile.cpp
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/IHardware.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/OPL.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/OPLFactory.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/Timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/Chip.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/scummvm/dosbox/DosBoxOPL.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/scummvm/dosbox/dbopl/dbopl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/scummvm/mame/MameOPL2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/scummvm/mame/mame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/scummvm/nuked/NukedOPL3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/scummvm/nuked/opl3.c
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/mame/ymfm/ymfm_adpcm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/mame/ymfm/ymfm_fm.ipp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/mame/ymfm/ymfm_opl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/mame/ymfm/ymfm_pcm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/mame/MameOPL3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/mame/opl3/ymf262.cpp
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/woody/WoodyOPL.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/woody/OPLChip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/woody/SurroundOPL.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/opl/woody/WoodyEmuOPL.cpp
    # --- #
    $<$<BOOL:${USE_MT32}>:${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/mt32/MT32.cpp>
    $<$<BOOL:${USE_MT32}>:${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/mt32/MT32ReportHandler.cpp>
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/hardware/PCSpeaker.cpp
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/utils/algorithms.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/utils/constants.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/utils/endianness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/utils/opl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/utils/sound.cpp
    # --- #
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/utils/ILogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/utils/Logger.cpp
    $<$<BOOL:${USE_SDL2}>:${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/utils/sdl2/Logger.cpp>
    $<$<BOOL:${USE_SDL3}>:${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/utils/sdl3/Logger.cpp>
    $<$<BOOL:${USE_SPDLOG}>:${CMAKE_CURRENT_SOURCE_DIR}/src/HyperSonicDrivers/utils/spdlog/Logger.cpp>
    # --- #
)

target_link_libraries(${LIB_NAME}
    PRIVATE
        ${LIB_SDL}
        ${LIB_SPDLOG}
    PUBLIC
        ${LIB_MT32EMU}
        rtaudio
)

if(USE_SPDLOG)
    install(
        DIRECTORY ${fmt_SOURCE_DIR}/include/fmt/
        DESTINATION include/fmt
    )

    set_target_properties(fmt PROPERTIES PUBLIC_HEADER "")
    install(
        TARGETS
            fmt
            spdlog
            rtaudio
            ${LIB_NAME}
        EXPORT ${LIB_CONF}
        RUNTIME DESTINATION static/bin
        LIBRARY DESTINATION static/lib
        ARCHIVE DESTINATION static/lib
        PUBLIC_HEADER DESTINATION static/include
    )
else()

    install(
        TARGETS
            rtaudio
            ${LIB_NAME}
        EXPORT ${LIB_CONF}
        RUNTIME DESTINATION static/bin
        LIBRARY DESTINATION static/lib
        ARCHIVE DESTINATION static/lib
        PUBLIC_HEADER DESTINATION static/include
    )
endif()

install(EXPORT ${LIB_CONF}
    DESTINATION static/cmake
    NAMESPACE HyperSonicDrivers::
)

if(USE_SDL3 OR USE_SDL2)
    install(FILES
        $<TARGET_FILE:${LIB_SDL}>
        DESTINATION static/lib
    )

    if(BUILD_EXAMPLES)
        install(FILES
            $<TARGET_FILE:${LIB_SDL}>
            DESTINATION "static/examples"
    )
    endif()
endif()

unset(LIB_NAME)
unset(LIB_CONF)
unset(LIB_VER)
##############################################################################

# Add source to this project's executable.
if(USE_SDL2)
    add_executable(sdl2-hyper-sonic-drivers "sdl2-hyper-sonic-drivers.cpp")
    target_link_libraries(sdl2-hyper-sonic-drivers PRIVATE
        $<TARGET_NAME_IF_EXISTS:${LIB_SDL}main> hyper-sonic-drivers-static)
endif()

if(BUILD_TESTING)
    add_subdirectory("test")
endif()

if (BUILD_EXAMPLES)
    add_subdirectory("examples")
endif()
