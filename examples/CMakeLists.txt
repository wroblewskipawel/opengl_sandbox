cmake_minimum_required(VERSION 3.16)

function(copy_directory TARGET_NAME SOURCE_DIR DEST_DIR)
    add_custom_command(
        TARGET ${TARGET_NAME}
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SOURCE_DIR}
        ${DEST_DIR}
    )
endfunction()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/pbr_viewer/)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tile_map/)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/model_animation/)
