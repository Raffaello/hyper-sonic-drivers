cmake_minimum_required (VERSION 3.21)

find_package(GTest 1.11 CONFIG REQUIRED)
include(GoogleTest)
include(CMakeParseArguments)

macro(macro_test) # ${ARGN} link libraries
    
    set(prefix TEST)
    #set(flags DEFAULT)
    set(singleValues EXE)
    set(multiValues FILES FIXTURES LINKS_PRIVATE)
    
    cmake_parse_arguments(${prefix} "${flags}" "${singleValues}" "${multiValues}" ${ARGN})
    #message("EXE: ${TEST_EXE}")
    #message("FILES: ${TEST_FILES}")
    #message("FIXTURES: ${TEST_FIXTURES}")
    #message("LINKS_PRIVATE: ${TEST_LINKS_PRIVATE}")
    add_executable (${TEST_EXE} ${TEST_FILES})
    target_include_directories(${TEST_EXE} PUBLIC
        ${CMAKE_SOURCE_DIR}/hyper-sonic-drivers/src
        ${CMAKE_SOURCE_DIR}/hyper-sonic-drivers/test
    )
    #target_link_libraries(${TEST_EXE} PRIVATE ${TEST_LINKS})
    target_link_libraries(${TEST_EXE} PRIVATE GTest::gtest GTest::gmock)
    add_custom_command(TARGET ${TEST_EXE} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:GTest::gtest_main> $<TARGET_FILE_DIR:${TEST_EXE}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:GTest::gtest> $<TARGET_FILE_DIR:${TEST_EXE}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:GTest::gmock> $<TARGET_FILE_DIR:${TEST_EXE}>
    )

    set(fixtures ${TEST_FIXTURES})
    list(LENGTH fixtures num_fixtures)
    unset(fixtures)
    if(num_fixtures EQUAL 0)
        message("${TEST_EXE}: No fixtures declared")
    else()
        message("${TEST_EXE} Fixtures: ${TEST_FIXTURES}")
        foreach(fixture_i ${TEST_FIXTURES})
            message("copy fixture ${fixture_i}")
            add_custom_command(TARGET ${TEST_EXE} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${CMAKE_CURRENT_SOURCE_DIR}/${fixture_i}
                        ${CMAKE_CURRENT_BINARY_DIR}/${fixture_i}
            )
        endforeach()
    endif()

    target_link_libraries(${TEST_EXE} PRIVATE ${TEST_LINKS_PRIVATE})
    copy_runtime_deps(${TEST_EXE})

    #add_test(
    #    NAME ${TEST_EXE}
    #    COMMAND ${TEST_EXE} --gtest_output=xml:report.xml
    #)

    gtest_discover_tests(${TEST_EXE} PROPERTIES TEST_DISCOVERY_TIMEOUT 600 DISCOVERY_TIMEOUT 600)
endmacro()

macro_test(
    EXE TestWaveGenerators
    FILES "TestWaveGenerators.cpp"
)

macro_test(
    EXE TestAlgorithms
    FILES "utils/TestAlgorithms.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestOpl
    FILES "utils/TestOpl.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestSound
    FILES "utils/TestSound.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestPCSpeaker
    FILES "hardware/TestPCSpeaker.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestEndianness
    FILES "utils/TestEndianness.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestFile
    FILES "files/TestFile.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES
        "../fixtures/empty.empty"
        "../fixtures/empty.zero"
)

macro_test(
    EXE TestADLFile
    FILES "files/westwood/TestADLFile.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES 
        "../fixtures/EOBSOUND.ADL" # v1
        "../fixtures/DUNE19.ADL"   # v2
        "../fixtures/DUNE0.ADL"    # v2
        "../fixtures/LOREINTR.ADL" # v3
)

macro_test(
    EXE TestMIDFile
    FILES "files/TestMIDFile.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES
        "../fixtures/MI_intro.mid"
        "../fixtures/midifile_sample.mid"
)

macro_test(
    EXE TestMUSFile
    FILES "files/dmx/TestMUSFile.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES
        "../fixtures/D_E1M1.MUS"
        #"../fixtures/D_E1M1.mid"
)

macro_test(
    EXE TestOP2File
    FILES "files/dmx/TestOP2File.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES
        "../fixtures/GENMIDI.OP2"
)

macro_test(
    EXE TestIDevice
    FILES "devices/TestIDevice.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES
)

macro_test(
    EXE TestOplDevice
    FILES "devices/TestOpl.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestAdlib
    FILES "devices/TestAdlib.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestSbPro
    FILES "devices/TestSbPro.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestSbPro2
    FILES "devices/TestSbPro2.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestMIDDriver
    FILES "drivers/TestMIDDriver.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES 
        "../fixtures/midifile_sample.mid"
)

macro_test(
    EXE TestIAudioDriver
    FILES "drivers/TestIAudioDriver.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES 
)

macro_test(
    EXE TestPCMDriver
    FILES "drivers/TestPCMDriver.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES 
)

macro_test(
    EXE TestMameOPL2
    FILES "hardware/opl/scummvm/mame/TestMameOPL2.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES "../fixtures/mame-22050-16bit-440Hz.dat"
)

macro_test(
    EXE TestDOSBoxOPL
    FILES "hardware/opl/scummvm/dosbox/TestDosBoxOPL.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES
        #"../fixtures/mame-22050-16bit-440Hz.dat"
)

macro_test(
    EXE TestNukedOPL3
    FILES "hardware/opl/scummvm/nuked/TestNukedOPL3.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES
        #"../fixtures/mame-22050-16bit-440Hz.dat"
)

macro_test(
    EXE TestWoodyOPL
    FILES "hardware/opl/woody/TestWoodyOPL.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES
        #"../fixtures/mame-22050-16bit-440Hz.dat"
)

macro_test(
    EXE TestOPLFactory
    FILES 
        "hardware/opl/TestOPLFactory.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestTimer
    FILES 
        "hardware/opl/TestTimer.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestChip
    FILES 
        "hardware/opl/TestChip.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestADLDriver
    FILES "drivers/westwood/TestADLDriver.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestWAVFile
    FILES "files/TestWAVFile.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES
        "../fixtures/Wav_868kb.wav" # format 1
)

macro_test(
    EXE TestVOCFile
    FILES "files/TestVOCFile.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES
        "../fixtures/VSCREAM1.VOC"
        "../fixtures/DUNE.VOC"
)

macro_test(
    EXE TestOplVoice
    FILES "drivers/midi/opl/TestOplVoice.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES
        "../fixtures/GENMIDI.OP2"
)

if(USE_SDL3)
    macro_test(
        EXE TestRenderer
        FILES "audio/sdl3/TestRenderer.cpp"
        LINKS_PRIVATE hyper-sonic-drivers-static
        FIXTURES
            "../fixtures/test_renderer_adlib_mame2.wav"
            "../fixtures/test_renderer_sbpro2_dosbox.wav"
            "../fixtures/DUNE0.ADL"
    )
elseif(USE_SDL2)
    macro_test(
        EXE TestRenderer
        FILES "audio/sdl2/TestRenderer.cpp"
        LINKS_PRIVATE hyper-sonic-drivers-static
        FIXTURES
            "../fixtures/test_renderer_adlib_mame2.wav"
            "../fixtures/test_renderer_sbpro2_dosbox.wav"
            "../fixtures/DUNE0.ADL"
    )
endif()

##### future tests ###
macro_test(
    EXE TestIFFFile
    FILES "files/TestIFFFile.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
)

macro_test(
    EXE TestIMFFile
    FILES "files/TestIMFFile.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES
        "../fixtures/02.imf"
)

macro_test(
    EXE TestXMIFile
    FILES "files/miles/TestXMIFile.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES 
        "../fixtures/header.xmi"
        "../fixtures/header_no_xdir.xmi"
        "../fixtures/AIL2_14_DEMO.XMI"
)

macro_test(
    EXE TestAILFileDriver
    FILES "files/miles/TestAILDriverFile.cpp"
    LINKS_PRIVATE hyper-sonic-drivers-static
    FIXTURES 
        "../fixtures/ADLIB.ADV"
        "../fixtures/ADLIBG.ADV"
        #"fixtures/ALGDIG.ADV"
        "../fixtures/MT32MPU.ADV"
        #"fixtures/PASDIG.ADV"
        #"fixtures/PASFM.ADV"
        #"fixtures/PASOPL.ADV"
        "../fixtures/PCSPKR.ADV"
        #"fixtures/SBDIG.ADV"
        "../fixtures/SBFM.ADV"
        "../fixtures/SBP1FM.ADV"
        "../fixtures/SBP2FM.ADV"
        #"fixtures/SBPDIG.ADV"
        "../fixtures/TANDY.ADV"
)


