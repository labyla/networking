add_library(networking-client STATIC
    src/client.cpp
)

target_include_directories(networking-client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(networking-client PUBLIC
    steam
    buffer
)

target_link_libraries(networking-client PRIVATE
    fmt
)

# set(APP_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)

# add_custom_command(TARGET networking-client POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#         ${APP_RESOURCES_DIR}/shaders
#         $<TARGET_FILE_DIR:opengl-engine>/shaders
#     COMMENT "Copying application resources"
# )
