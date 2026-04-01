#ifndef PLOP_SRC_UI_COLOURS_HPP
#define PLOP_SRC_UI_COLOURS_HPP

#include <juce_gui_basics/juce_gui_basics.h>

namespace plop::colours {

	// ---- Accent / interactive -----------------------------------------------
	const auto accentBlue         = ::juce::Colour( 0xff4fc3f7 ); // cyan — waveform, editor outline
	const auto accentOrange       = ::juce::Colour( 0xffE4884E ); // orange — waveform, editor outline
	const auto btnAccentColour    = ::juce::Colour( 0xffab47bc ); // purple — toggle-on buttons
	const auto btnAccentColourAlt = ::juce::Colour( 0xff66bb6a ); // green — play/pause toggle-on
	const auto addAccent          = ::juce::Colour( 0xff66cc66 ); // green — add button text, add-mode border
	const auto removeAccent       = ::juce::Colour( 0xffff6666 ); // red — remove button text
	const auto replaceModeAccent  = ::juce::Colour( 0xffcc6666 ); // red — replace-mode border in pattern picker

	// ---- Backgrounds --------------------------------------------------------
	const auto panelBg     = ::juce::Colour( 0xffE4E4E4 ); // note/CC list panel background
	const auto lcdBgColour = panelBg; // ::juce::Colour( 0xff0b0b18 ); // darkest — CC display even lanes
	const auto orbitalBg   = panelBg; //::juce::Colour( 0xff1a1a2e ); // orbital display canvas
	//const auto settingsBg      = ::juce::Colour( 0xff181828 ); // settings panel
	const auto patternPickerBg = ::juce::Colour( 0xff0f0f1c ); // pattern picker panel
	const auto rowAlt          = ::juce::Colour( 0xffD4D4D4 ); // alternating row tint
	const auto inputBg         = ::juce::Colour( 0xff2a2a44 ); // text editor bg, active field highlight
	const auto addBg           = ::juce::Colour( 0xff223322 ); // add button / add-mode toggle bg
	const auto removeBg        = ::juce::Colour( 0xff553333 ); // remove button bg
	const auto replaceModeBg   = ::juce::Colour( 0xff221a1a ); // replace-mode toggle bg in pattern picker

	const auto darkestGrey = ::juce::Colour( 0xff323232 );

	// ---- Headers & borders --------------------------------------------------
	const auto noteHeaderBg = panelBg;                      // ::juce::Colour( 0xff555577 ); // note list panel header
	const auto borderLine   = ::juce::Colour( 0xff333344 ); // separators and borders
	const auto offWhite     = ::juce::Colour( 0xff888899 ); // column labels, dim text
	const auto subtleGrey   = ::juce::Colour( 0xff222233 ); // beat grid lines in CC display

	const auto defText = darkestGrey;
	// ---- Orbital display ----------------------------------------------------
	const auto orbitalDotFallback = ::juce::Colour( 0xffaaaacc ); // dot when no colour assigned

	// ---- MIDI export button -------------------------------------------------
	const auto midiExportBtnNormal = ::juce::Colour( 0xff1a3a6a );
	const auto midiExportBtnHover  = ::juce::Colour( 0xff2255aa );
	const auto midiExportBtnBorder = ::juce::Colour( 0xff88bbff );

	// ---- Voice / pattern palette (shared by OrbitalDisplay & PatternPicker) -
	const auto paletteBlue   = ::juce::Colour( 0xff4fc3f7 ); // == accentBlue
	const auto paletteRed    = ::juce::Colour( 0xffef5350 );
	const auto paletteGreen  = ::juce::Colour( 0xff66bb6a ); // == btnAccentColourAlt
	const auto paletteOrange = ::juce::Colour( 0xffffa726 );
	const auto palettePurple = ::juce::Colour( 0xffab47bc ); // == btnAccentColour
	const auto paletteTeal   = ::juce::Colour( 0xff26c6da );
	const auto palettePink   = ::juce::Colour( 0xffec407a );
	const auto paletteBrown  = ::juce::Colour( 0xff8d6e63 );

} // namespace plop::colours

#endif // PLOP_SRC_UI_COLOURS_HPP
