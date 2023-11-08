cmake_minimum_required(VERSION 3.20)
project(picture_client)

add_executable(client client.c)
target_link_libraries(client PUBLIC -lglut -lGL -pthread)
