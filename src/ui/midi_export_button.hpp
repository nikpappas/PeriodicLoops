#ifndef PLOP_SRC_UI_MIDI_EXPORT_BUTTON_HPP
#define PLOP_SRC_UI_MIDI_EXPORT_BUTTON_HPP

#include <cmath>
#include <vector>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "music/midi.hpp"
#include "ui/colours.hpp"

namespace plop::ui {

	/// Generates a MIDI file for `bars` bars into the OS temp directory and returns the file.
	inline ::juce::File generateMidiExport( const std::vector<PeriodicNote> &notes,
	                                        const std::vector<PeriodicCC>   &ccs,
	                                        float                            bpm,
	                                        int                              bars        = 16,
	                                        int                              beatsPerBar = 4 ) {
		const int   ticksPerBeat = 480;
		const float totalBeats   = static_cast<float>( bars * beatsPerBar );

		::juce::MidiFile midiFile;
		midiFile.setTicksPerQuarterNote( ticksPerBeat );

		::juce::MidiMessageSequence track;

		// Tempo meta-event
		track.addEvent( ::juce::MidiMessage::tempoMetaEvent( static_cast<int>( 60'000'000.0 / bpm ) ), 0 );

		// Notes
		for ( const auto &note : notes ) {
			if ( note.period <= 0.0f )
				continue;
			for ( float beat = note.offset; beat < totalBeats; beat += note.period ) {
				const int tickOn  = static_cast<int>( beat * ticksPerBeat );
				const int tickOff = static_cast<int>( ( beat + note.duration ) * ticksPerBeat );
				track.addEvent( ::juce::MidiMessage::noteOn( note.channel + 1, note.pitch, static_cast<uint8_t>( 100 ) ), tickOn );
				track.addEvent( ::juce::MidiMessage::noteOff( note.channel + 1, note.pitch ), tickOff );
			}
		}

		// CCs — sampled at every 0.1 beats (≈ 100 Hz at 100 bpm)
		const int ccTickStep = ticksPerBeat / 10;
		for ( const auto &cc : ccs ) {
			if ( cc.period <= 0.0f )
				continue;
			const int totalTicks = static_cast<int>( totalBeats * ticksPerBeat );
			for ( int tick = 0; tick < totalTicks; tick += ccTickStep ) {
				const float beat  = static_cast<float>( tick ) / ticksPerBeat;
				const float phase = ( beat + cc.offset ) / cc.period;
				const int   val   = static_cast<int>( 127.0f * ( 0.5f + 0.5f * std::sin( 2.0f * 3.14159265f * phase ) ) );
				track.addEvent( ::juce::MidiMessage::controllerEvent( cc.channel + 1, cc.number, val ), tick );
			}
		}

		track.updateMatchedPairs();
		midiFile.addTrack( track );

		auto file = ::juce::File::getSpecialLocation( ::juce::File::tempDirectory ).getChildFile( "periodic_loop_export.mid" );
		file.deleteFile(); // FileOutputStream appends by default — delete first to overwrite
		::juce::FileOutputStream stream( file );
		if ( stream.openedOk() )
			midiFile.writeTo( stream );

		return file;
	}

	/// A button that initiates an OS-level drag-and-drop of a generated MIDI file
	/// when the user drags it.
	class MidiExportButton : public ::juce::Component {

	 public:
		MidiExportButton( const std::function<::juce::File()> &onGenerateMidi ) : mOnGenerateMidi( onGenerateMidi ) {
		}

		const std::function<::juce::File()> mOnGenerateMidi;

		void paint( ::juce::Graphics &g ) override {
			const auto b = getLocalBounds().toFloat();
			g.setColour( mHovered ? colours::midiExportBtnHover : colours::midiExportBtnNormal );
			g.fillRoundedRectangle( b, 4.0f );
			g.setColour( colours::midiExportBtnBorder );
			g.drawRoundedRectangle( b.reduced( 0.5f ), 4.0f, 1.0f );
			g.setColour( ::juce::Colours::white );
			g.setFont( 12.0f );
			g.drawText( "Drag MIDI", getLocalBounds(), ::juce::Justification::centred );
		}

		void mouseEnter( const ::juce::MouseEvent & ) override {
			mHovered = true;
			repaint();
		}
		void mouseExit( const ::juce::MouseEvent & ) override {
			mHovered = false;
			repaint();
		}
		void mouseDown( const ::juce::MouseEvent & ) override {
		}

		void mouseDrag( const ::juce::MouseEvent &e ) override {
			if ( !mDragStarted && e.getDistanceFromDragStart() > 5 ) {
				mDragStarted    = true;
				const auto file = mOnGenerateMidi();
				if ( file.existsAsFile() )
					::juce::DragAndDropContainer::performExternalDragDropOfFiles( { file.getFullPathName() }, false, this );
			}
		}

		void mouseUp( const ::juce::MouseEvent & ) override {
			mDragStarted = false;
		}

	 private:
		bool mHovered     = false;
		bool mDragStarted = false;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( MidiExportButton )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_MIDI_EXPORT_BUTTON_HPP
