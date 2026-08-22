cmake_minimum_required(VERSION 3.22.1)
project(coefficient_root_dance C)

set(NATIVE_GLUE_DIR "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")

add_library(native_app_glue STATIC
    "${NATIVE_GLUE_DIR}/android_native_app_glue.c"
)
target_include_directories(native_app_glue PUBLIC "${NATIVE_GLUE_DIR}")

add_library(coefficient_root_dance SHARED
    dance.c
    polynomial.c
)
target_compile_features(coefficient_root_dance PRIVATE c_std_11)
target_compile_options(coefficient_root_dance PRIVATE -Wall -Wextra -Wpedantic)
target_link_options(coefficient_root_dance PRIVATE -u ANativeActivity_onCreate)
target_link_libraries(coefficient_root_dance
    native_app_glue
    android
    log
    EGL
    GLESv3
    m
)
