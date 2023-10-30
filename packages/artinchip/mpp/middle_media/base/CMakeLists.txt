set(BASE_MSG
    message/aic_message.c
)

set(BASE_STREAM
    stream/aic_stream.c
    stream/file/aic_file_stream.c
)

set(BASE_PARSER
    parser/aic_parser.c
    parser/mov/aic_mov_parser.c
    parser/mov/mov_tags.c
    parser/mov/mov.c
    parser/rawdata/aic_raw_parser.c
)

set(BASE_RENDER
    render/aic_video_render.c
)

set(BASE_AUDIO_RENDER
    render/aic_audio_render.c
)

add_library(openmax_base SHARED
    ${BASE_MSG}
    ${BASE_STREAM}
    ${BASE_PARSER}
    ${BASE_RENDER}
    ${BASE_AUDIO_RENDER}
)

target_link_libraries(openmax_base base asound)

target_include_directories(openmax_base PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser/mov"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser/rawdata"
    "${CMAKE_CURRENT_SOURCE_DIR}/stream/file"
)

if(DEFINED CMAKE_INSTALL_FULL_LIBDIR)
    install(TARGETS openmax_base RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")
endif() # CMAKE_INSTALL_FULL_LIBDIR

