cmake_minimum_required(VERSION 3.22)
project(MicEffector VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 로컬 JUCE 경로 연동
add_subdirectory("C:/JUCE" JUCE)

# JUCE 전용 애플리케이션 타겟 생성
juce_add_gui_app(MicEffector
    PRODUCT_NAME "MicEffector"
    COMPANY_NAME "raculus"
    COMPANY_COPYRIGHT "Copyright (c) 2026 raculus"
    COMPANY_WEBSITE ""
    BUNDLE_ID "com.raculus.miceffector"
    VERSION 1.0.0
    ICON_BIG   "${CMAKE_CURRENT_SOURCE_DIR}/source/icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/source/icon.png"
)

target_sources(MicEffector PRIVATE
    source/main.cpp
    source/AudioPipeline.cpp
    source/PluginManagerComponent.cpp
    source/Localization.cpp
)

# VST3 호스팅 활성화 및 매크로 설정
target_compile_definitions(MicEffector PRIVATE
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
)

# MSVC 인코딩 경고 방지
if (MSVC)
    target_compile_options(MicEffector PRIVATE /utf-8)
endif()

# JUCE 모듈 링크 (juce_generate_juce_header보다 먼저 와야 함)
target_link_libraries(MicEffector PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_events
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    Shell32.lib
    User32.lib
)

juce_generate_juce_header(MicEffector)