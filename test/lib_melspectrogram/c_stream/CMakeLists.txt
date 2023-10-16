set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

## If XCORE_TARGET hasn't been specified, default to XCORE-AI-EXPLORER
if( NOT DEFINED XCORE_TARGET )
  set( XCORE_TARGET XCORE-AI-EXPLORER )
endif()


if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_executable(fwk_voice_melspectrogram_c_stream)

    file(GLOB_RECURSE SOURCES_C "src/*.c")

    target_sources(fwk_voice_melspectrogram_c_stream PRIVATE ${SOURCES_C} src/top_level.xc)
    
    option(MEL_SMALL "Switches application to demonstrating the small model" ON)
    option(TEST_BURN "Switches on burn mode, ensuring all cores are scheduled" OFF)
    option(COMPILE_ALGORITHM "Compiles mel algorithm into application to gather memory use" ON)
    option(USE_QUANT_EQ "Uses programmed equation before int8_t conversion" ON)
    option(CONVERT_DB "Converts output of Mel spec. to dB before mean subtraction and quantisation" ON)
    option(SUBTRACT_MEAN "Subtracts the mean of the Mel spec output after dB conversion but before quantisation" ON)

    set(APP_COMPILER_FLAGS
        -O3
        -mno-dual-issue
        -g
        -report
        -fxscope
        -mcmodel=large
        -Wno-xcore-fptrgroup    
        -target=${XCORE_TARGET}
    )
    target_link_libraries(fwk_voice_melspectrogram_c_stream 
        PUBLIC
            fwk_voice::melspectrogram
            )

    target_compile_options(fwk_voice_melspectrogram_c_stream
        PRIVATE ${APP_COMPILER_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/config.xscope)

    if(COMPILE_ALGORITHM)
        message("COMPILE ALGORITHM")
        target_compile_definitions(fwk_voice_melspectrogram_c_stream PRIVATE COMPILE_ALGORITHM=1)

        if(MEL_SMALL)
            message("SMALL MEL")
            target_compile_definitions(fwk_voice_melspectrogram_c_stream PRIVATE MEL_SMALL=1)
        else()
            message("LARGE MEL")
            target_compile_definitions(fwk_voice_melspectrogram_c_stream PRIVATE MEL_SMALL=0)
        endif()

        if(USE_QUANT_EQ)
            message("USE QUANTISATION EQUATION")
            target_compile_definitions(fwk_voice_melspectrogram_c_stream PRIVATE USE_QUANT_EQ=1)
        else()
            message("DO NOT USE QUANTISATION EQUATION")
            target_compile_definitions(fwk_voice_melspectrogram_c_stream PRIVATE USE_QUANT_EQ=0)
        endif()

        if(CONVERT_DB)
            message("CONVERT TO DB")
            target_compile_definitions(fwk_voice_melspectrogram_c_stream PRIVATE CONVERT_DB=1)
        else()
            message("DO NOT CONVERT TO DB")
            target_compile_definitions(fwk_voice_melspectrogram_c_stream PRIVATE CONVERT_DB=0)
        endif()

        if(SUBTRACT_MEAN)
            message("SUBTRACT MEAN")
            target_compile_definitions(fwk_voice_melspectrogram_c_stream PRIVATE SUBTRACT_MEAN=1)
        else()
            message("DO NOT SUBTRACT MEAN")
            target_compile_definitions(fwk_voice_melspectrogram_c_stream PRIVATE SUBTRACT_MEAN=0)
        endif()

    else()
        message("DO NOT COMPILE ALGORITHM")
        target_compile_definitions(fwk_voice_melspectrogram_c_stream PRIVATE COMPILE_ALGORITHM=0)
    endif()

    if(TEST_BURN)
    message("TEST BURN")
    target_compile_definitions(fwk_voice_melspectrogram_c_stream PRIVATE TEST_BURN=1)
    else()
    message("DO NOT BURN")
    target_compile_definitions(fwk_voice_melspectrogram_c_stream PRIVATE TEST_BURN=0)
    endif()

    
    
    target_link_options(fwk_voice_melspectrogram_c_stream
        PRIVATE
            -w            
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
endif()