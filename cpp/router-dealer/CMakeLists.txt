
add_executable(
    demo_router
    ${CMAKE_CURRENT_SOURCE_DIR}/demo_router.cpp
)

target_link_libraries(
    demo_router
    ${LINK_LIBS} 
)

set(DEALER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/dealer_node.cpp)

add_executable(
    demo_dealer_server
    ${DEALER_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/demo_dealer_server.cpp
)

target_link_libraries(
    demo_dealer_server
    ${LINK_LIBS} 
)

add_executable(
    demo_dealer_client
    ${DEALER_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/demo_dealer_client.cpp
)

target_link_libraries(
    demo_dealer_client
    ${LINK_LIBS} 
)



