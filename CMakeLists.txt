cmake_minimum_required(VERSION 3.23)
project(GlcdFontCalculator)

add_executable(GlcdFontCalculator main.c)

target_link_libraries(GlcdFontCalculator user32.lib gdiplus.lib shlwapi.lib)
