set(COMPONENT_SRCS "BoardDev34.cpp" "HTTPC2Server.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_PRIV_REQUIRES BoardManager Debouncer ESP32SimpleOTA esp_http_server)
set(COMPONENT_EMBED_FILES "indexc.html")

register_component()
