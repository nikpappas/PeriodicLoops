#ifndef PLOP_SRC_UI_P_LOOPS_UI_HPP
#define PLOP_SRC_UI_P_LOOPS_UI_HPP

#include <juce_audio_processors/juce_audio_processors.h>

#include "processor/periodic_loops.hpp"

namespace plop::ui {

	class p_loops_ui : public ::juce::AudioProcessorEditor {
	 private:
		::plop::p_loops::p_loops &m_plugin_instance_ref;

	 public:
		explicit p_loops_ui( ::plop::p_loops::p_loops &owner );
		~p_loops_ui();
	};

	p_loops_ui::p_loops_ui( ::plop::p_loops::p_loops &owner ) :
			  ::juce::AudioProcessorEditor( owner ), m_plugin_instance_ref( owner ) {

		setSize( 800, 600 );
		setResizable( false, false );
	}

	p_loops_ui::~p_loops_ui() {
	}

} // namespace plop::ui

#endif // PLOP_SRC_UI_P_LOOPS_UI_HPP
