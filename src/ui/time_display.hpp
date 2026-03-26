#ifndef PLOP_SRC_UI_TIME_DISPLAY_HPP
#define PLOP_SRC_UI_TIME_DISPLAY_HPP

#include <juce_gui_basics/juce_gui_basics.h>

namespace plop::ui {

	class TimeDisplay : public ::juce::Component {
	 public:
		TimeDisplay() = default;

		void setTime( int64_t t ) {
			mTime = t;
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( ::juce::Colours::black );
			g.setColour( ::juce::Colours::white );
			g.setFont( 16.0f );
			g.drawFittedText( "Time: " + ::juce::String( mTime ), getLocalBounds(), ::juce::Justification::centred, 1 );
		}

	 private:
		int64_t mTime = 0;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( TimeDisplay )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_TIME_DISPLAY_HPP
