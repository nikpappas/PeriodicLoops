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
using ::juce::AudioProcessor;
using ::juce::AudioProcessorEditor;
using ::juce::File;
using ::juce::MemoryBlock;
using ::juce::MidiBuffer;
using ::juce::MidiMessage;
using ::juce::PluginHostType;

using ::std::ceil;
using ::std::clamp;
using ::std::initializer_list;
using ::std::make_shared;
using ::std::memory_order_acquire;
using ::std::string;
using ::std::to_string;
using ::std::vector;

namespace ranges = ::std::ranges;

using namespace ::plop::p_loops;

p_loops::p_loops() : AudioProcessor( p_loops::get_midifx_bus_layout() ) {

	// DRUMS
	for ( const PeriodicNote &note : initializer_list<PeriodicNote>{

			  { .pitch = 46, .period = 16.0f, .offset = 0.1f, .duration = 1.0f, .channel = 2 },
			  { .pitch = 36, .period = 2.0f, .offset = 0, .duration = 1.0f, .channel = 2 },
			  { .pitch = 42, .period = 1.0f, .offset = 0.5, .duration = 1.0f, .channel = 2 },

			  { .pitch = 72, .period = 16.0f, .offset = 0, .duration = 2.0f, .channel = 0 },
			  { .pitch = 67, .period = 16.0f, .offset = 2 * 0.33f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 65, .period = 16.0f, .offset = 2 * 0.5f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 66, .period = 16.0f, .offset = 2 * 1.35f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 65, .period = 16.0f, .offset = 2 * 1.35f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 72, .period = 16.0f, .offset = 2 * 2, .duration = 2.0f, .channel = 0 },
			  { .pitch = 67, .period = 16.0f, .offset = 2 * 2.33f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 65, .period = 16.0f, .offset = 2 * 2.5f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 60, .period = 16.0f, .offset = 2 * 3.35f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 63, .period = 16.0f, .offset = 2 * 3.5f, .duration = 0.5f, .channel = 0 },

			  { .pitch = 60, .period = 4.0f, .offset = 2 * 0.0f, .duration = 0.5f, .channel = 1 },
			  { .pitch = 63, .period = 4.0f, .offset = 2 * 1.5f, .duration = 0.5f, .channel = 1 },
			  { .pitch = 65, .period = 4.0f, .offset = 2 * 0.0f, .duration = 0.5f, .channel = 1 },
			  { .pitch = 67, .period = 8.0f, .offset = 2 * 1.25f, .duration = 0.5f, .channel = 1 },
			  { .pitch = 67, .period = 12.0f, .offset = 2 * 0.0f, .duration = 0.5f, .channel = 1 },
			  { .pitch = 69, .period = 16.0f, .offset = 2 * 1.5f, .duration = 0.5f, .channel = 1 },
			  { .pitch = 72, .period = 16.0f, .offset = 2 * 1.5f, .duration = 0.5f, .channel = 1 },

			} )
		addNote( note );

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

void p_loops::addNote( const PeriodicNote &note ) {
	swapBuffer( [ & ]( NoteBuffer &buf ) {
		if ( buf.count < MAX_NOTES )
			buf.notes[ buf.count++ ] = note;
	} );
	m_ui_notes.push_back( note );
}

void p_loops::removeNote( int index ) {
	if ( index < 0 || index >= static_cast<int>( m_ui_notes.size() ) )
		return;
	swapBuffer( [ index ]( NoteBuffer &buf ) {
		if ( index < buf.count ) {
			for ( int i = index; i < buf.count - 1; ++i )
				buf.notes[ i ] = buf.notes[ i + 1 ];
			--buf.count;
		}
	} );
	m_ui_notes.erase( m_ui_notes.begin() + index );
}

void p_loops::updateNote( int index, const PeriodicNote &note ) {
	if ( index < 0 || index >= static_cast<int>( m_ui_notes.size() ) )
		return;
	swapBuffer( [ index, &note ]( NoteBuffer &buf ) {
		if ( index < buf.count )
			buf.notes[ index ] = note;
	} );
	m_ui_notes[ index ] = note;
}

void p_loops::reset() {
	// Use this method as the place to clear any delay lines, buffers, etc, as it
	// means there's been a break in the audio's continuity.
}

void p_loops::processBlock( AudioBuffer<float> &buffer, MidiBuffer &midi ) {
	buffer.clear();
	midi.clear();

	const auto        numSamples = buffer.getNumSamples();
	const NoteBuffer &noteBuf    = m_note_buf[ m_active_buf.load( memory_order_acquire ) ];

	if ( noteBuf.count == 0 ) {
		time += numSamples;
		return;
	}

	// Get BPM from host transport if available
	if ( auto *playH = getPlayHead() ) {
		juce::AudioPlayHead::CurrentPositionInfo posInfo;
		if ( playH->getCurrentPosition( posInfo ) ) {
			bpm = static_cast<float>( posInfo.bpm );
		}
	}

	const float   samplesPerBeat = static_cast<float>( mSampleRate ) * 60.0f / bpm;
	const int64_t timeSnapshot   = time.load();
	const float   blockStartBeat = static_cast<float>( timeSnapshot ) / samplesPerBeat;
	const float   blockEndBeat   = static_cast<float>( timeSnapshot + numSamples ) / samplesPerBeat;

	// NoteOff pass — must come before NoteOn so same-sample ordering is correct
	for ( int ni = 0; ni < noteBuf.count; ++ni ) {
		const auto &note = noteBuf.notes[ ni ];
		if ( note.period <= 0.0f )
			continue;

		auto n = static_cast<int>( ceil( ( blockStartBeat - note.duration - note.offset ) / note.period - 1e-6f ) );
		if ( n < 0 )
			n = 0;

		while ( true ) {
			const float offBeat = static_cast<float>( n ) * note.period + note.duration + note.offset;
			if ( offBeat >= blockEndBeat )
				break;
			if ( offBeat >= blockStartBeat ) {
				const int offset = clamp( static_cast<int>( ( offBeat - blockStartBeat ) * samplesPerBeat ), 0, numSamples - 1 );
				midi.addEvent( MidiMessage::noteOff( note.channel + 1, note.pitch ), offset );
			}
			++n;
		}
	}

	// NoteOn pass
	for ( int ni = 0; ni < noteBuf.count; ++ni ) {
		const auto &note = noteBuf.notes[ ni ];
		if ( note.period <= 0.0f )
			continue;

		auto n = static_cast<int>( ceil( ( blockStartBeat - note.offset ) / note.period - 1e-6f ) );
		if ( n < 0 )
			n = 0;

		while ( true ) {
			const float onBeat = static_cast<float>( n ) * note.period + note.offset;
			if ( onBeat >= blockEndBeat )
				break;
			const int offset = clamp( static_cast<int>( ( onBeat - blockStartBeat ) * samplesPerBeat ), 0, numSamples - 1 );
			midi.addEvent( MidiMessage::noteOn( note.channel + 1, note.pitch, (uint8_t)100 ), offset );
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
