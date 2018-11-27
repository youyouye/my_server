
set(
    Http_SRCS
    http_base.cpp
    http_codec.cpp
    http_server.cpp
	http_status_code.cpp
)

add_library(http ${Http_SRCS})
target_link_libraries(http log server)
set_target_properties(http PROPERTIES COMPILE_FLAGS "-std=c++11")

install(TARGETS http DESTINATION lib)

set(HEADERS
	http_base.h
	http_codec.h
	http_server.h
	http_status_code.h
)

install(FILES ${HEADERS} DESTINATION include/http)

