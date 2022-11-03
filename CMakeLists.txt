cmake_minimum_required(VERSION 3.23)
project(GlcdFontCalculator)

include_directories(inc)
add_executable(GlcdFontCalculator
        src/main.c
        src/edit_utils.c)

target_link_libraries(GlcdFontCalculator
        user32.lib
        gdiplus.lib
        shlwapi.lib)
