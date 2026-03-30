#ifndef PLOP_SRC_UI_UI_CONSTANTS_HPP
#define PLOP_SRC_UI_UI_CONSTANTS_HPP

#include <juce_gui_basics/juce_gui_basics.h>

namespace plop::ui {

	/// Corner radius applied to every button-shaped element in the UI.
	inline constexpr float BTN_CORNER_RADIUS = 4.0f;

	/// Type scale — use these instead of raw float literals everywhere.
	inline constexpr float FONT_SM = 11.0f; ///< Labels, column headers, glyph buttons
	inline constexpr float FONT_MD = 12.0f; ///< Empty-state / secondary messages
	inline constexpr float FONT_LG = 13.0f; ///< Row content, action buttons, panel headers
	inline constexpr float FONT_XL = 16.0f; ///< Prominent display values (time, BPM)

	/// LookAndFeel override that applies BTN_CORNER_RADIUS to all TextButtons.
	class PlopLookAndFeel : public ::juce::LookAndFeel_V4 {
	 public:
		void drawButtonBackground( ::juce::Graphics         &g,
		                           ::juce::Button           &button,
		                           const ::juce::Colour     &backgroundColour,
		                           bool                      isMouseOverButton,
		                           bool                      isButtonDown ) override {
			const auto bounds     = button.getLocalBounds().toFloat().reduced( 0.5f );
			auto       baseColour = backgroundColour;
			if ( isButtonDown )
				baseColour = baseColour.contrasting( 0.2f );
			else if ( isMouseOverButton )
				baseColour = baseColour.brighter( 0.05f );

			g.setColour( baseColour );
			g.fillRoundedRectangle( bounds, BTN_CORNER_RADIUS );
			g.setColour( button.findColour( ::juce::ComboBox::outlineColourId ) );
			g.drawRoundedRectangle( bounds, BTN_CORNER_RADIUS, 1.0f );
		}
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_UI_CONSTANTS_HPP
