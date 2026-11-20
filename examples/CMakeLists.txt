# ---- modular examples (directory-per-example) --------------------------------

add_subdirectory(chat)

if(TOBUS_ENABLE_HTTP)
    add_subdirectory(http_relay)
endif()
