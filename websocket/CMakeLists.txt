
set(
    Websocket_SRCS
    codec.cpp
    websocket_server.cpp
)

add_library(websocket ${Websocket_SRCS})
target_link_libraries(websocket log server)
set_target_properties(websocket PROPERTIES COMPILE_FLAGS "-std=c++11")

install(TARGETS websocket DESTINATION lib)

set(HEADERS
    codec.h
    websocket_server.h
)

install(FILES ${HEADERS} DESTINATION include/websocket)

