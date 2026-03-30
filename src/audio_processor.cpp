#include <juce_audio_processors/juce_audio_processors.h>

#include "processor/periodic_loops.hpp"

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
	return new ::plop::p_loops::PLoops();
}
