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
using ::std::min;
using ::std::string;
using ::std::to_string;
using ::std::vector;

using ::plop::utils::beatsToSamples;

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

	for ( const PeriodicCC &cc : initializer_list<PeriodicCC>{
			  { .number = 32, .period = 4.0f, .offset = 0.1f, .channel = 0 },
			  { .number = 42, .period = 4.0f, .offset = 0.1f, .channel = 0 },
			} )
		addCc( cc );

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

void p_loops::prepareToPlay( double sampleRate, int prmBlockSize ) {
	mSampleRate = sampleRate;
	mBlockSize  = prmBlockSize;
	time        = 0;
}

void p_loops::releaseResources() {
}

void p_loops::addNote( const PeriodicNote &note ) {
	swapBuffer( [ & ]( NoteBuffer &buf ) {
		if ( buf.count < MAX_NOTES )
			buf.notes[ buf.count++ ] = note;
	} );
	mUiNotes.push_back( note );
}

void p_loops::addCc( const PeriodicCC &cc ) {
	swapCCBuffer( [ & ]( CCBuffer &buf ) {
		if ( buf.count < MAX_NOTES )
			buf.cc[ buf.count++ ] = cc;
	} );
	mUiCcs.push_back( cc );
}

void p_loops::removeCc( int index ) {
	if ( index < 0 || index >= static_cast<int>( mUiCcs.size() ) )
		return;
	swapCCBuffer( [ index ]( CCBuffer &buf ) {
		if ( index < buf.count ) {
			for ( int i = index; i < buf.count - 1; ++i )
				buf.cc[ i ] = buf.cc[ i + 1 ];
			--buf.count;
		}
	} );
	mUiCcs.erase( mUiCcs.begin() + index );
}

void p_loops::updateCc( int index, const PeriodicCC &cc ) {
	if ( index < 0 || index >= static_cast<int>( mUiCcs.size() ) )
		return;
	swapCCBuffer( [ index, &cc ]( CCBuffer &buf ) {
		if ( index < buf.count )
			buf.cc[ index ] = cc;
	} );
	mUiCcs[ index ] = cc;
}

void p_loops::removeNote( int index ) {
	if ( index < 0 || index >= static_cast<int>( mUiNotes.size() ) )
		return;
	swapBuffer( [ index ]( NoteBuffer &buf ) {
		if ( index < buf.count ) {
			for ( int i = index; i < buf.count - 1; ++i )
				buf.notes[ i ] = buf.notes[ i + 1 ];
			--buf.count;
		}
	} );
	mUiNotes.erase( mUiNotes.begin() + index );
}

void p_loops::updateNote( int index, const PeriodicNote &note ) {
	if ( index < 0 || index >= static_cast<int>( mUiNotes.size() ) )
		return;
	swapBuffer( [ index, &note ]( NoteBuffer &buf ) {
		if ( index < buf.count )
			buf.notes[ index ] = note;
	} );
	mUiNotes[ index ] = note;
}

void p_loops::reset() {
	// Use this method as the place to clear any delay lines, buffers, etc, as it
	// means there's been a break in the audio's continuity.
}

void p_loops::processBlock( AudioBuffer<float> &buffer, MidiBuffer &midi ) {
	buffer.clear();
	midi.clear();

	const auto        numSamples = buffer.getNumSamples();
	const NoteBuffer &noteBuf    = mNoteBuf[ mActiveNoteBuf.load( memory_order_acquire ) ];

	if ( noteBuf.count == 0 ) {
		time += numSamples;
		return;
	}

	// Derive blockStartBeat from host playhead when available, fall back to sample counter
	float ppqPosition = -1.0f;
	if ( auto *playH = getPlayHead() ) {
		auto posInfo = playH->getPosition();
		if ( posInfo.hasValue() ) {
			if ( posInfo->getBpm().hasValue() ) {
				bpm.store( static_cast<float>( *posInfo->getBpm() ) );
			}
			// If it's a plugin only produce midi when the playhead is playing
			if ( !posInfo->getIsPlaying() && wrapperType != wrapperType_Standalone ) {
				return;
			}
			if ( posInfo->getIsPlaying() && posInfo->getPpqPosition().hasValue() ) {
				ppqPosition = static_cast<float>( *posInfo->getPpqPosition() );
			}
		}
	}

	const float samplesPerBeat = static_cast<float>( mSampleRate ) * 60.0f / bpm.load();
	const float blockStartBeat = ppqPosition >= 0.0f ? ppqPosition : static_cast<float>( time.load() ) / samplesPerBeat;
	const float blockEndBeat   = blockStartBeat + static_cast<float>( numSamples ) / samplesPerBeat;

	if ( ppqPosition >= 0.0f )
		time.store( static_cast<int64_t>( blockStartBeat * samplesPerBeat ) );

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

	const int ccSamplingStep = static_cast<int>( mSampleRate / 100.0 ); // 100 Hz max

	const CCBuffer &ccBuf = mCcBuf[ mActiveCcBuf.load( memory_order_acquire ) ];
	// CC pass
	if ( ccBuf.count > 0 ) {
		for ( int i = 0; i < numSamples; i += ccSamplingStep ) {
			for ( int ni = 0; ni < ccBuf.count; ++ni ) {
				const auto &cc = ccBuf.cc[ ni ];
				if ( cc.period <= 0.0f ) {
					continue;
				}

				midi.addEvent(
				  MidiMessage::controllerEvent(
					 cc.channel + 1,
					 cc.number,
					 static_cast<int>(
						127.0f * ( 0.5f + 0.5f * sin( 2 * 3.14 * ( time + i + cc.offset * samplesPerBeat ) / ( cc.period * samplesPerBeat ) ) ) ) ),
				  time + i );
			}
		}
	}

	time += numSamples;
}

void p_loops::processBlock( AudioBuffer<double> &, MidiBuffer & ) {
	pl_debug( "TODO: Double precision" );
}

void p_loops::getStateInformation( MemoryBlock &destData ) {
	juce::XmlElement root( "PeriodicLoopState" );

	auto *notesEl = root.createNewChildElement( "Notes" );
	for ( const auto &note : mUiNotes ) {
		auto *el = notesEl->createNewChildElement( "Note" );
		el->setAttribute( "pitch", note.pitch );
		el->setAttribute( "period", note.period );
		el->setAttribute( "offset", note.offset );
		el->setAttribute( "duration", note.duration );
		el->setAttribute( "channel", note.channel );
	}

	auto *ccsEl = root.createNewChildElement( "CCs" );
	for ( const auto &cc : mUiCcs ) {
		auto *el = ccsEl->createNewChildElement( "CC" );
		el->setAttribute( "number", cc.number );
		el->setAttribute( "period", cc.period );
		el->setAttribute( "offset", cc.offset );
		el->setAttribute( "channel", cc.channel );
	}

	copyXmlToBinary( root, destData );
	pl_debug( "getStateInformation: saved " + to_string( mUiNotes.size() ) + " notes, " + to_string( mUiCcs.size() ) + " CCs" );
}

void p_loops::setStateInformation( const void *data, int sizeInBytes ) {
	auto xml = getXmlFromBinary( data, sizeInBytes );
	if ( !xml || xml->getTagName() != "PeriodicLoopState" ) {
		pl_error( "setStateInformation: invalid or missing state XML" );
		return;
	}

	const auto *notesEl = xml->getChildByName( "Notes" );
	if ( !notesEl ) {
		pl_error( "setStateInformation: missing Notes element" );
		return;
	}

	// Clear existing notes
	while ( !mUiNotes.empty() )
		removeNote( 0 );

	for ( const auto *el : notesEl->getChildIterator() ) {
		PeriodicNote note{
			.pitch    = el->getIntAttribute( "pitch", 60 ),
			.period   = static_cast<float>( el->getDoubleAttribute( "period", 1.0 ) ),
			.offset   = static_cast<float>( el->getDoubleAttribute( "offset", 0.0 ) ),
			.duration = static_cast<float>( el->getDoubleAttribute( "duration", 0.5 ) ),
			.channel  = el->getIntAttribute( "channel", 0 ),
		};
		addNote( note );
	}

	// Clear existing CCs
	while ( !mUiCcs.empty() )
		removeCc( 0 );

	if ( const auto *ccsEl = xml->getChildByName( "CCs" ) ) {
		for ( const auto *el : ccsEl->getChildIterator() ) {
			PeriodicCC cc{
				.number  = el->getIntAttribute( "number", 1 ),
				.period  = static_cast<float>( el->getDoubleAttribute( "period", 1.0 ) ),
				.offset  = static_cast<float>( el->getDoubleAttribute( "offset", 0.0 ) ),
				.channel = el->getIntAttribute( "channel", 0 ),
			};
			addCc( cc );
		}
	}

	pl_debug( "setStateInformation: loaded " + to_string( mUiNotes.size() ) + " notes, " + to_string( mUiCcs.size() ) + " CCs" );
}

File p_loops::log_file() {
	return utils::get_pl_data_folder( true ).getChildFile( string{ utils::PLUGIN_NAME } + utils::LOG_EXTENSION );
}
