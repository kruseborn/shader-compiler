list(APPEND DEPS spirv-cross-glsl)
if(UNIX)
	list(APPEND DEPS stdc++fs)
endif()

mg_cc_executable(
    NAME
        spirv-reflection
    SRCS
        "spirv-reflection.h"
        "create-shader-structs.cpp"
        "parser.cpp"
        "main.cpp"
    COPTS
        ${CPP_FLAGS}
    DEPS
        ${DEPS}
)