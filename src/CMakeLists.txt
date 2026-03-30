

include_directories(.)

juce_add_plugin(
    PeriodicLoops
    VERSION 0.0.1
    PLUGIN_MANUFACTURER_CODE NIKP # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE PLOP # A unique four-character plugin id with at least one upper-case character
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Periodic Loops"
    COMPANY_NAME "NPSCSMPC"
    BUNDLE_ID "com.nikpappas"

    HARDENED_RUNTIME_ENABLED TRUE

    AU_MAIN_TYPE kAudioUnitType_MIDIProcessor
    COPY_PLUGIN_AFTER_BUILD FALSE
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    $<IF:$<PLATFORM_ID:Linux>,NEEDS_CURL TRUE,>
)

target_sources(PeriodicLoops PRIVATE
    audio_processor.cpp
    processor/periodic_loops.cpp
)

target_link_libraries(PeriodicLoops PRIVATE
    juce::juce_core
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_processors
    juce::juce_audio_utils
    # juce::juce_audio_device
)


target_compile_definitions(PeriodicLoops
    PRIVATE
    JUCE_WEB_BROWSER=0 # If JUCE_WEB_BROWSER isn't set to 0, ensure the `juce_add_plugin` call has `NEEDS_WEB_BROWSER TRUE`
    JUCE_USE_CURL=$<PLATFORM_ID:Linux> # If JUCE_USE_CURL isn't set to 0, ensure the `juce_add_plugin` call has `NEEDS_CURL TRUE`
    JUCE_VST3_CAN_REPLACE_VST2=0
)

np_prepare_target(PeriodicLoops)
np_add_compiler_warnings(PeriodicLoops)
