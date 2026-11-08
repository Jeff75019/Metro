file(GLOB SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
idf_component_register(SRCS ${SRCS} INCLUDE_DIRS "." REQUIRES arduino LovyanGFX ArduinoJson esp_wifi esp_netif esp_event)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
