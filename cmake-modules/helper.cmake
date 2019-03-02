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