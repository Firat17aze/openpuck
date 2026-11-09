# puck_core: device-independent game logic. No ESP-IDF dependencies, so it
# builds both as an ESP-IDF component and as a plain library for host tests.
set(PUCK_CORE_SRCS
    src/game.c
    src/save_policy.c
    src/state_codec.c
    src/touch_input.c
    src/timer.c
    src/history.c
    src/settings.c
    src/battery.c
    src/color.c
    src/number.c)

if(ESP_PLATFORM)
    idf_component_register(SRCS ${PUCK_CORE_SRCS} INCLUDE_DIRS "include")
else()
    add_library(puck_core STATIC ${PUCK_CORE_SRCS})
    target_include_directories(puck_core PUBLIC include)
    target_link_libraries(puck_core PUBLIC m)
endif()
