#include "periodic_loops.hpp"
#include "logging/logging.hpp"
#include "ui/p_loops_ui.hpp"
#include "utils/constants.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <juce_core/juce_core.h>

using ::juce::AudioBuffer;
using ::juce::AudioChannelSet;
using ::juce::AudioPluginInstance;
using ::juce::AudioProcessor;
using ::juce::AudioProcessorEditor;
using ::juce::File;
using ::juce::Graphics;
using ::juce::Identifier;
using ::juce::MemoryBlock;
using ::juce::MidiBuffer;
using ::juce::MidiMessage;
using ::juce::PluginHostType;
using ::juce::SystemStats;
using ::juce::ValueTree;
using ::juce::XmlElement;

using ::std::make_shared;
using ::std::next;
using ::std::shared_ptr;
using ::std::string;
using ::std::unique_ptr;
using ::std::vector;
using ::std::string_view_literals::operator""sv;

namespace ranges = ::std::ranges;

constexpr auto SPEC_XML_TAG         = "dstate"sv;
constexpr auto UI_XML_TAG           = "dstateUI"sv;
constexpr auto PLUGIN_OTHER_XML_TAG = "dstatePlugin"sv;

using namespace ::plop::p_loops;

p_loops::p_loops() :
		  AudioProcessor( p_loops::get_midifx_bus_layout() )
{
	notes.add( { 36, 4.0f, 2.0f } );
	notes.add( { 46, 1.0f, 0.5f } );
	pl_info( "Plugin Started" );
}

p_loops::~p_loops() noexcept {
	pl_info( "Done. Bye bye." );
}

AudioProcessorEditor *p_loops::createEditor() {
	pl_debug( "Create editor." );

	try {
		return new ui::p_loops_ui( *this );
	} catch ( ... ) {
		pl_error( "Could not create editor" );
		throw;
	}
}

bool p_loops::isBusesLayoutSupported( const BusesLayout &layouts ) const {
	if ( PluginHostType().isProTools() ) {
		return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
	}
	return true;
}

void p_loops::prepareToPlay( double sampleRate, [[maybe_unused]] int block_size ) {
	mSampleRate = sampleRate;
	time        = 0;
}

void p_loops::releaseResources() {
}

void p_loops::reset() {
	// Use this method as the place to clear any delay lines, buffers, etc, as it
	// means there's been a break in the audio's continuity.
}

void p_loops::processBlock( AudioBuffer<float> &buffer, MidiBuffer &midi ) {
	buffer.clear();
	midi.clear();

	const auto numSamples = buffer.getNumSamples();

	if ( notes.isEmpty() ) {
		time += numSamples;
		return;
	}

	// Get BPM from host transport if available
	if ( auto *playHead = getPlayHead() ) {
		juce::AudioPlayHead::CurrentPositionInfo posInfo;
		if ( playHead->getCurrentPosition( posInfo ) ) {
			bpm = static_cast<float>( posInfo.bpm );
		}
	}

	const float samplesPerBeat = static_cast<float>( mSampleRate ) * 60.0f / bpm;
	const float blockStartBeat = static_cast<float>( time ) / samplesPerBeat;
	const float blockEndBeat   = static_cast<float>( time + numSamples ) / samplesPerBeat;

	// NoteOff pass — must come before NoteOn so same-sample ordering is correct
	for ( const auto &note : notes ) {
		if ( note.period <= 0.0f ) continue;

		auto n = static_cast<int>( std::ceil( ( blockStartBeat - note.duration ) / note.period - 1e-6f ) );
		if ( n < 0 ) n = 0;

		while ( true ) {
			const float offBeat = static_cast<float>( n ) * note.period + note.duration;
			if ( offBeat >= blockEndBeat ) break;
			if ( offBeat >= blockStartBeat ) {
				const int offset = std::clamp( static_cast<int>( ( offBeat - blockStartBeat ) * samplesPerBeat ), 0, numSamples - 1 );
				midi.addEvent( MidiMessage::noteOff( 1, note.pitch ), offset );
			}
			++n;
		}
	}

	// NoteOn pass
	for ( const auto &note : notes ) {
		if ( note.period <= 0.0f ) continue;

		auto n = static_cast<int>( std::ceil( blockStartBeat / note.period - 1e-6f ) );
		if ( n < 0 ) n = 0;

		while ( true ) {
			const float onBeat = static_cast<float>( n ) * note.period;
			if ( onBeat >= blockEndBeat ) break;
			const int offset = std::clamp( static_cast<int>( ( onBeat - blockStartBeat ) * samplesPerBeat ), 0, numSamples - 1 );
			midi.addEvent( MidiMessage::noteOn( 1, note.pitch, (uint8_t) 100 ), offset );
			++n;
		}
	}

	time += numSamples;
}

void p_loops::processBlock( AudioBuffer<double> &, MidiBuffer & ) {
	pl_debug( "TODO: Double precision" );
}

void p_loops::getStateInformation( MemoryBlock &destData ) {
	pl_debug( "getStateInformation" );
}

void p_loops::setStateInformation( const void *data, int sizeInBytes ) {
	pl_debug( "setStateInformation" );
}

File p_loops::log_file() {
	return utils::get_pl_data_folder( true ).getChildFile( string{ utils::PLUGIN_NAME } + utils::LOG_EXTENSION );
}
