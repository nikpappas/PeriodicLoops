#ifndef PLOP_SRC_UI_TIME_DISPLAY_HPP
#define PLOP_SRC_UI_TIME_DISPLAY_HPP

#include <juce_gui_basics/juce_gui_basics.h>

namespace plop::ui {

	class TimeDisplay : public ::juce::Component {
	 public:
		TimeDisplay() = default;

		void setTime( int64_t t ) {
			m_time = t;
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( ::juce::Colours::black );
			g.setColour( ::juce::Colours::white );
			g.setFont( 16.0f );
			g.drawFittedText( "Time: " + ::juce::String( m_time ), getLocalBounds(), ::juce::Justification::centred, 1 );
		}

	 private:
		int64_t m_time = 0;
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_TIME_DISPLAY_HPP
