#ifndef PLOP_SRC_UI_P_LOOPS_UI_HPP
#define PLOP_SRC_UI_P_LOOPS_UI_HPP

#include <cmath>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "processor/periodic_loops.hpp"
#include "ui/note_list_panel.hpp"
#include "ui/orbital_display.hpp"
#include "ui/time_display.hpp"

namespace plop::ui {

	class p_loops_ui
			  : public ::juce::AudioProcessorEditor
			  , private ::juce::Timer
			  , private ::juce::ChangeListener {
	 private:
		::plop::p_loops::p_loops &m_plugin_instance_ref;
		TimeDisplay               m_time_display;
		OrbitalDisplay            m_orbital_display;
		NoteListPanel             m_note_list_panel;
		int64_t                   m_last_time = 0;

		std::vector<::juce::Colour>                            m_note_colours;
		::juce::Component::SafePointer<::juce::ColourSelector> m_active_selector;
		int                                                    m_editing_index = -1;

	 public:
		explicit p_loops_ui( ::plop::p_loops::p_loops &owner );
		~p_loops_ui();

	 private:
		::juce::Colour nextPaletteColour() const {
			static const ::juce::Colour palette[] = {
				::juce::Colour( 0xff4fc3f7 ), ::juce::Colour( 0xffef5350 ), ::juce::Colour( 0xff66bb6a ),
				::juce::Colour( 0xffffa726 ), ::juce::Colour( 0xffab47bc ), ::juce::Colour( 0xff26c6da ),
				::juce::Colour( 0xffec407a ), ::juce::Colour( 0xff8d6e63 ),
			};
			return palette[ m_note_colours.size() % std::size( palette ) ];
		}

		void changeListenerCallback( ::juce::ChangeBroadcaster *source ) override {
			if ( m_active_selector != nullptr && source == m_active_selector.getComponent() && m_editing_index >= 0 ) {
				m_note_colours[ m_editing_index ] = m_active_selector->getCurrentColour();
			}
		}

		void openColourPicker( int index, ::juce::Rectangle<int> screenBounds ) {
			if ( index < 0 || index >= static_cast<int>( m_note_colours.size() ) )
				return;
			if ( m_active_selector != nullptr )
				m_active_selector->removeChangeListener( this );

			auto selector = std::make_unique<::juce::ColourSelector>( ::juce::ColourSelector::showColourAtTop
			                                                          | ::juce::ColourSelector::showSliders
			                                                          | ::juce::ColourSelector::showColourspace );
			selector->setSize( 300, 380 );
			selector->setCurrentColour( m_note_colours[ index ] );
			selector->addChangeListener( this );

			m_active_selector = selector.get();
			m_editing_index   = index;

			::juce::CallOutBox::launchAsynchronously( std::move( selector ), screenBounds, nullptr );
		}

		void timerCallback() override {
			const int64_t currentTime    = m_plugin_instance_ref.getTime();
			const float   bpm            = m_plugin_instance_ref.getBpm();
			const float   sampleRate     = static_cast<float>( m_plugin_instance_ref.getSampleRate() );
			const auto   &notes          = m_plugin_instance_ref.getNotes();
			const float   samplesPerBeat = sampleRate * 60.0f / bpm;
			const float   currentBeat    = static_cast<float>( currentTime ) / samplesPerBeat;
			const float   lastBeat       = static_cast<float>( m_last_time ) / samplesPerBeat;

			// Compute pitch range for radius normalisation
			int minPitch = 127, maxPitch = 0;
			for ( const auto &note : notes ) {
				minPitch = std::min( minPitch, note.pitch );
				maxPitch = std::max( maxPitch, note.pitch );
			}
			const int pitchRange = maxPitch - minPitch;

			std::vector<OrbitalDisplay::VoiceState> states;
			states.reserve( static_cast<size_t>( notes.size() ) );
			for ( const auto &note : notes ) {
				const float phase       = std::fmod( currentBeat, note.period ) / note.period - note.offset / note.period;
				const bool  triggered   = std::floor( ( currentBeat - note.offset ) / note.period )
				                          > std::floor( ( lastBeat - note.offset ) / note.period );
				const float trackRadius = pitchRange > 0 ? static_cast<float>( note.pitch - minPitch ) / pitchRange : 0.5f;
				states.push_back( { phase, triggered, trackRadius } );
			}

			m_orbital_display.setVoices( std::move( states ) );
			m_orbital_display.setColours( m_note_colours );
			m_orbital_display.repaint();

			m_note_list_panel.setNotes( { notes.begin(), notes.end() } );
			m_note_list_panel.setColours( m_note_colours );
			m_note_list_panel.repaint();

			m_time_display.setTime( currentTime );
			m_time_display.repaint();

			m_last_time = currentTime;
		}
	};

	p_loops_ui::p_loops_ui( ::plop::p_loops::p_loops &owner ) :
			  ::juce::AudioProcessorEditor( owner ), m_plugin_instance_ref( owner ) {
		// Assign initial colours for pre-loaded notes
		for ( size_t i = 0; i < owner.getNotes().size(); ++i )
			m_note_colours.push_back( nextPaletteColour() );

		m_note_list_panel.onColourSwatchClicked = [ this ]( int index, ::juce::Rectangle<int> screenBounds ) {
			openColourPicker( index, screenBounds );
		};

		m_note_list_panel.onAddNote = [ this ] {
			constexpr PeriodicNote defaultNote{ .pitch = 60, .period = 1.0f, .offset = 0.0f, .duration = 0.5f, .channel = 0 };
			m_plugin_instance_ref.addNote( defaultNote );
			m_note_colours.push_back( nextPaletteColour() );
		};

		m_note_list_panel.onNoteChanged = [ this ]( int index, PeriodicNote note ) {
			m_plugin_instance_ref.updateNote( index, note );
		};

		m_note_list_panel.onRemoveNote = [ this ]( int index ) {
			m_plugin_instance_ref.removeNote( index );
			if ( index < static_cast<int>( m_note_colours.size() ) )
				m_note_colours.erase( m_note_colours.begin() + index );
		};

		setSize( 800, 600 );

		constexpr int panel_w = 240;
		constexpr int top_h   = 50;

		addAndMakeVisible( m_time_display );
		m_time_display.setBounds( 10, 10, 200, 30 );

		addAndMakeVisible( m_orbital_display );
		m_orbital_display.setBounds( 0, top_h, 800 - panel_w, 600 - top_h );

		addAndMakeVisible( m_note_list_panel );
		m_note_list_panel.setBounds( 800 - panel_w, top_h, panel_w, 600 - top_h );

		setResizable( false, false );
		startTimerHz( 30 );
	}

	p_loops_ui::~p_loops_ui() {
		if ( m_active_selector != nullptr )
			m_active_selector->removeChangeListener( this );
		stopTimer();
	}

} // namespace plop::ui

#endif // PLOP_SRC_UI_P_LOOPS_UI_HPP
