set(c_SOURCE_FILES
    "TheBoard.cpp"
    "HTTPSrv.cpp"
)

set(c_PRIVATE_REQUIREMENTS
    ESP32BoardManager
    Debouncer
)

idf_component_register(
    SRCS ${c_SOURCE_FILES}
    INCLUDE_DIRS "."
    PRIV_REQUIRES ${c_PRIVATE_REQUIREMENTS}
)
