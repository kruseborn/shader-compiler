function(mg_cc_library)
    cmake_parse_arguments(MG_CC
        "" # list of names of the boolean arguments (only defined ones will be true)
        "NAME;TYPE;" # list of names of mono-valued arguments
        "SRCS;DEFS;COPTS;DEPS;DEPS_DIR;INCLUDE_DIR" # list of names of multi-valued arguments (output variables are lists)
        ${ARGN} # arguments of the function to parse, here we take the all original ones
    )
    set(NAME ${MG_CC_NAME})

    # Check if this is a header-only library
    set(MG_CC_SRCS "${MG_CC_SRCS}")
    list(FILTER MG_CC_SRCS EXCLUDE REGEX "(.*\\.h|.*\\.inl)")
    if ("${MG_CC_SRCS}" STREQUAL "")
        set(MG_CC_IS_INTERFACE 1)
    else()
        set(MG_CC_IS_INTERFACE 0)
    endif()

    # create filters/groups same as folders on disc for IDE
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${MG_CC_SRCS})
    source_group("" FILES ${MG_CC_SRCS})

    if(NOT ${MG_CC_IS_INTERFACE})
        add_library(${NAME} ${MG_CC_TYPE} "")

        target_sources(${NAME} PRIVATE ${MG_CC_SRCS})
        target_include_directories(${NAME} PUBLIC ${MG_CC_INCLUDE_DIR})
        target_include_directories(${NAME} SYSTEM PRIVATE ${MG_CC_DEPS_DIR})
        target_compile_definitions(${NAME} PRIVATE ${MG_CC_DEFS})
        target_compile_features(${NAME} PUBLIC "cxx_std_17")

        install(TARGETS ${NAME} RUNTIME DESTINATION ${RAYCARE_EXE_PATH})

        target_link_libraries(${NAME} PUBLIC ${MG_CC_DEPS})

        # INTERFACE libraries can't have the CXX_STANDARD property set
        set_property(TARGET ${NAME} PROPERTY CXX_STANDARD ${MG_CXX_STANDARD})
        set_property(TARGET ${NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

        target_compile_options(${NAME} PRIVATE ${MG_CC_COPTS})

    else()
        # Generating header-only library
        add_library(${NAME} INTERFACE)
        target_include_directories(${NAME} INTERFACE ${MG_CC_INCLUDE_DIR})
        target_link_libraries(${NAME} INTERFACE ${MG_CC_DEPS})
    endif()
endfunction()

function(mg_cc_executable)
    cmake_parse_arguments(MG_CC
        "" # list of names of the boolean arguments (only defined ones will be true)
        "NAME;" # list of names of mono-valued arguments
        "SRCS;COPTS;DEPS;" # list of names of multi-valued arguments (output variables are lists)
        ${ARGN} # arguments of the function to parse, here we take the all original ones
    )
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF) # -std=c++11 rather than -std=gnu++11

    message(${MG_CC_SRCS})

    set(NAME ${MG_CC_NAME})
    add_executable(
        ${NAME}
        ${MG_CC_SRCS}
    )
    target_link_libraries(${NAME} PUBLIC ${MG_CC_DEPS})
    target_compile_options(${NAME} PRIVATE ${MG_CC_COPTS})
endfunction()