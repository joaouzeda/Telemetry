file(GLOB_RECURSE SOURCES ".cpp" ".c")

idf_component_register(
    SRCS "main.cpp" "RealSync.cpp" "user_interface.cpp"
    INCLUDE_DIRS "."
    REQUIRES arduino Adafruit_BusIO Adafruit_GFX_Library Adafruit_SSD1306
)