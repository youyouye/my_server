set (
    SERVER_SRCS
    accept.cpp
    buffer.cpp
    channel.cpp
    callback.hpp
    client.cpp
    connection.cpp
    connector.cpp
    epoll.cpp
    event_loop.cpp
    server.cpp
    socket.cpp
    socket_op.cpp
    timestamp.cpp
    crypto.hpp
)
add_library(server ${SERVER_SRCS})
target_link_libraries(server log pthread ssl crypto)
set_target_properties(server PROPERTIES COMPILE_FLAGS "-std=c++11")

install(TARGETS server DESTINATION bin)

set(HEADERS
    accept.h
    buffer.h
    channel.h
    client.h
    connection.h
    endian.h
    epoll.h
    event_loop.h
    server.h
    socket_op.h
    timestamp.h
)
install(FILES ${HEADERS} DESTINATION include/server/)
