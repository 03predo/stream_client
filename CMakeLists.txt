cmake_minimum_required(VERSION 3.20)
project(stream_client)

add_compile_definitions(IMG_HEIGHT=240
                        IMG_WIDTH=320
                        BYTES_PER_PIXEL=2
                        FPS=30
                        PORT=2848
                        SERVER_IP="10.0.0.38"
                        )
                    

add_executable(stream_client main.c graphics.c client.c)
target_link_libraries(stream_client PUBLIC -lglut -lGL -pthread)
add_custom_target(run COMMAND ${CMAKE_BINARY_DIR}/stream_client ${SERVER_IP})

add_executable(stats stats.c client.c)
add_custom_target(run_stats COMMAND ${CMAKE_BINARY_DIR}/stats)
