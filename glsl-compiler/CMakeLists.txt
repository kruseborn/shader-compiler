list(APPEND DEPS "")
if(UNIX)
	list(APPEND DEPS stdc++fs)
endif()

mg_cc_executable(
    NAME
        glsl-compiler
    SRCS
        main.cpp
		parsers.h
		parseRasterization.cpp
		parseRayTracer.cpp
		parseCompute.cpp
		utils.h
		utils.cpp
    COPTS
        ${CPP_FLAGS}
    DEPS
        ${DEPS}
)