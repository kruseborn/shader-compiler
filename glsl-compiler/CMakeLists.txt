mg_cc_executable(
    NAME
        shader-compiler
    SRCS
        main.cpp
    COPTS
        ${LLVM_FLAGS}
)