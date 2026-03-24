#ifndef PLOP_SRC_UI_P_LOOPS_UI_HPP
#define PLOP_SRC_UI_P_LOOPS_UI_HPP

#include <cmath>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "processor/periodic_loops.hpp"
#include "ui/orbital_display.hpp"
#include "ui/time_display.hpp"

namespace plop::ui {

	class p_loops_ui
	   : public ::juce::AudioProcessorEditor
	   , private ::juce::Timer {
	 private:
		::plop::p_loops::p_loops &m_plugin_instance_ref;
		TimeDisplay               m_time_display;
		OrbitalDisplay            m_orbital_display;
		int64_t                   m_last_time = 0;

	 public:
		explicit p_loops_ui( ::plop::p_loops::p_loops &owner );
		~p_loops_ui();

	 private:
		void timerCallback() override {
			const int64_t currentTime    = m_plugin_instance_ref.getTime();
			const float   bpm            = m_plugin_instance_ref.getBpm();
			const float   sampleRate     = static_cast<float>( m_plugin_instance_ref.getSampleRate() );
			const auto &  notes          = m_plugin_instance_ref.getNotes();
			const float   samplesPerBeat = sampleRate * 60.0f / bpm;
			const float   currentBeat    = static_cast<float>( currentTime ) / samplesPerBeat;
			const float   lastBeat       = static_cast<float>( m_last_time ) / samplesPerBeat;

			std::vector<OrbitalDisplay::VoiceState> states;
			states.reserve( static_cast<size_t>( notes.size() ) );
			for ( const auto &note : notes ) {
				const float phase     = std::fmod( currentBeat, note.period ) / note.period;
				const bool  triggered = std::floor( currentBeat / note.period ) > std::floor( lastBeat / note.period );
				states.push_back( { phase, triggered } );
			}

			m_orbital_display.setVoices( std::move( states ) );
			m_orbital_display.repaint();

			m_time_display.setTime( currentTime );
			m_time_display.repaint();

			m_last_time = currentTime;
		}
	};

	p_loops_ui::p_loops_ui( ::plop::p_loops::p_loops &owner ) :
			  ::juce::AudioProcessorEditor( owner ),
			  m_plugin_instance_ref( owner )
	{
		setSize( 800, 600 );

		addAndMakeVisible( m_time_display );
		m_time_display.setBounds( 10, 10, 200, 30 );

		addAndMakeVisible( m_orbital_display );
		m_orbital_display.setBounds( 0, 50, 800, 550 );

		setResizable( false, false );
		startTimerHz( 30 );
	}

	p_loops_ui::~p_loops_ui() {
		stopTimer();
	}

} // namespace plop::ui

#endif // PLOP_SRC_UI_P_LOOPS_UI_HPP
