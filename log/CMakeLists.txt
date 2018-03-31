set(log_SRCS
    Log.cpp 
)

add_library(log ${log_SRCS})
install(TARGETS log DESTINATION lib)
set_target_properties(log PROPERTIES COMPILE_FLAGS "-std=c++11")

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/log)

