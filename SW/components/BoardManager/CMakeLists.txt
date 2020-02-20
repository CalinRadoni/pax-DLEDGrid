set(COMPONENT_SRCS "Board.cpp" "Configuration.cpp" "Events.cpp" "HTTPConfigServer.cpp" "WiFiManager.cpp" "WiFiConfig.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")
set(COMPONENT_PRIV_REQUIRES nvs_flash esp_http_server)
set(COMPONENT_EMBED_FILES "index.html" "jquery.js")

register_component()
