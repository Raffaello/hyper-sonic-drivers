cmake_minimum_required (VERSION 3.21)

### Macro ###
macro(macro_example) # ${ARGN} link libraries
    set(prefix EXAMPLE)
    set(flags COPY_LIB NONE)
    set(singleValues EXE FILE)
    set(multiValues DEPS FIXTURES LINKS)

    include(CMakeParseArguments)
    cmake_parse_arguments(${prefix} "${flags}" "${singleValues}" "${multiValues}" ${ARGN})
    #message("${EXE} - ${FILE} - ${DEPS} - ${LINKS}")
    #message("EXE====${EXAMPLE_EXE}")
    #message("IS NONE: ${EXAMPLE_NONE}")
    #message("IS Copy_LIB: ${EXAMPLE_COPY_LIB}")
    #message("EXE: ${EXAMPLE_EXE}")
    #message("FILE: ${EXAMPLE_FILE}")
    #message("DEP: ${EXAMPLE_DEPS}")
    #message("LINKS: ${EXAMPLE_LINKS}")
    #message("FIXTURES: ${EXAMPLE_FIXTURES}")
    add_executable (${EXAMPLE_EXE} ${EXAMPLE_FILE})
    add_dependencies(${EXAMPLE_EXE} ${EXAMPLE_DEPS}) 
    target_link_libraries(${EXAMPLE_EXE} ${EXAMPLE_LINKS})
    if(${EXAMPLE_COPY_LIB})
        #message("===============EXAMPLE COPY LIB")
        foreach(TF IN ITEMS ${EXAMPLE_DEPS})
            #message("++++ TF= ${TF} ++++")
            add_custom_command(
                TARGET ${EXAMPLE_EXE} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${TF}> $<TARGET_FILE_DIR:${EXAMPLE_EXE}>
            )
        endforeach()
    endif()

    set(fixtures ${EXAMPLE_FIXTURES})
    list(LENGTH fixtures num_fixtures)
    unset(fixtures)
    if(num_fixtures EQUAL 0)
        message("${EXAMPLE_EXE}: No fixtures declared")
    else()
        message("${EXAMPLE_EXE} Fixtures: ${EXAMPLE_FIXTURES}")
        foreach(fixture_i ${EXAMPLE_FIXTURES})
            message("copy fixture ${fixture_i}")
            add_custom_command(TARGET ${EXAMPLE_EXE} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${CMAKE_CURRENT_SOURCE_DIR}/${fixture_i}
                        ${CMAKE_CURRENT_BINARY_DIR}
            )
            install(
                FILES ${fixture_i}
                DESTINATION "static/examples"
            )
        endforeach()
    endif()

    copy_runtime_deps(${EXAMPLE_EXE})

    install(FILES
        $<TARGET_FILE:${EXAMPLE_EXE}>
        DESTINATION "static/examples/" # TODO: works only if linked with static
    )
    install(FILES
        $<TARGET_FILE:spdlog::spdlog>
        $<TARGET_FILE:fmt::fmt>
        DESTINATION "static/examples"
    )
endmacro()

############################## Main Example ##################################
macro_example(
    EXE OPLExample
    FILE "opl-example.cpp"
    DEPS hyper-sonic-drivers-static
    LINKS
        PRIVATE
            $<TARGET_NAME_IF_EXISTS:${LIB_SDL}main>
            hyper-sonic-drivers-static
            spdlog::spdlog
    NONE
)

macro_example(
    EXE ADLExample
    FILE "adl-example.cpp"
    DEPS hyper-sonic-drivers-static
    LINKS
        PRIVATE
            $<TARGET_NAME_IF_EXISTS:${LIB_SDL}main>
            hyper-sonic-drivers-static
            spdlog::spdlog
    FIXTURES 
      "../test/fixtures/DUNE0.ADL"
      #"../test/fixtures/EOBSOUND.ADL"
      #"../test/fixtures/LOREINTR.ADL"
    NONE
)

macro_example(
    EXE IMFExample
    FILE "imf-example.cpp"
    DEPS hyper-sonic-drivers-static
    LINKS
        PRIVATE
            $<TARGET_NAME_IF_EXISTS:${LIB_SDL}main>
            hyper-sonic-drivers-static
            spdlog::spdlog
    FIXTURES 
      "../test/fixtures/02.imf"
    NONE
)

if(USE_SDL2)
    macro_example(
        EXE ADLPlay
        FILE "adl-play.cpp"
        DEPS hyper-sonic-drivers-static
        LINKS
            PRIVATE
                $<TARGET_NAME_IF_EXISTS:${LIB_SDL}main>
                hyper-sonic-drivers-static
                spdlog::spdlog
        FIXTURES 
        "../test/fixtures/DUNE0.ADL"
        #"../test/fixtures/EOBSOUND.ADL"
        #"../test/fixtures/LOREINTR.ADL"
        NONE
    )
endif()

macro_example(
    EXE PCMExample
    FILE "pcm-example.cpp"
    DEPS hyper-sonic-drivers-static
    LINKS
        PRIVATE
            $<TARGET_NAME_IF_EXISTS:${LIB_SDL}main>
            hyper-sonic-drivers-static
            spdlog::spdlog
    FIXTURES
      "../test/fixtures/Wav_868kb.wav"
      "../test/fixtures/DUNE.VOC"
    NONE
)

macro_example(
    EXE MIDexample
    FILE "mid-example.cpp"
    DEPS hyper-sonic-drivers-static
    LINKS
        PRIVATE
            $<TARGET_NAME_IF_EXISTS:${LIB_SDL}main>
            hyper-sonic-drivers-static
            spdlog::spdlog
    FIXTURES
      "../test/fixtures/MI_intro.mid"
      #"../test/fixtures/midifile_sample.mid"
    NONE
)

macro_example(
    EXE MUSexample
    FILE "mus-example.cpp"
    DEPS hyper-sonic-drivers-static
    LINKS
        PRIVATE
            $<TARGET_NAME_IF_EXISTS:${LIB_SDL}main>
            hyper-sonic-drivers-static
            spdlog::spdlog
    FIXTURES
      "../test/fixtures/D_E1M1.MUS"
      "../test/fixtures/GENMIDI.OP2"
    NONE
)

if (USE_MT32)
macro_example(
    EXE MT32example
    FILE "mt32-example.cpp"
    DEPS hyper-sonic-drivers-static
    LINKS
        PRIVATE
            $<TARGET_NAME_IF_EXISTS:${LIB_SDL}main>
            hyper-sonic-drivers-static
            spdlog::spdlog
    FIXTURES
      "../test/fixtures/MI_intro.mid"
    NONE
)
endif()
