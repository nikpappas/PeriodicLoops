#ifndef PLOP_SRC_UI_PATTERN_PICKER_HPP
#define PLOP_SRC_UI_PATTERN_PICKER_HPP

#include <cmath>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/midi.hpp"
#include "ui/colours.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	struct PatternDef {
		const char               *name;
		::juce::Colour            colour;
		std::vector<PeriodicNote> notes;
	};

	inline std::vector<PatternDef> makeBuiltinPatterns() {
		return {
			{ "Pulse", colours::paletteBlue, { { 60, 1.0f, 0.0f, 0.5f, 0 } } },
			{ "8ths", colours::paletteGreen, { { 60, 0.5f, 0.0f, 0.25f, 0 } } },
			{ "Off+On", colours::paletteOrange, { { 60, 1.0f, 0.0f, 0.25f, 0 }, { 64, 1.0f, 0.5f, 0.25f, 0 } } },
			{ "3:4", colours::palettePurple, { { 60, 4.0f / 3.0f, 0.0f, 0.15f, 0 }, { 64, 1.0f, 0.0f, 0.15f, 0 } } },
			{ "Waltz", colours::paletteRed, { { 60, 3.0f, 0.0f, 0.4f, 0 }, { 64, 3.0f, 1.0f, 0.3f, 0 }, { 67, 3.0f, 2.0f, 0.3f, 0 } } },
			{ "Kick+Hi",
			  colours::paletteBrown,
			  { { 36, 2.0f, 0.0f, 0.1f, 0 }, { 38, 2.0f, 1.0f, 0.1f, 0 }, { 42, 0.5f, 0.0f, 0.05f, 0 } } },
		};
	}

	/// A row of pattern swatches.  Click applies the pattern; the +/↺ toggle
	/// selects whether it adds to or replaces the current note list.
	class PatternPicker : public ::juce::Component {
	 public:
		using OnPickPattern = std::function<void( const std::vector<PeriodicNote> &, bool add )>;

		explicit PatternPicker( OnPickPattern onPickPattern ) :
				  mOnPickPattern( std::move( onPickPattern ) ), mPatterns( makeBuiltinPatterns() ) {
		}

		void paint( ::juce::Graphics &g ) override {
			const int w = getWidth();
			const int h = getHeight();

			// Background
			g.fillAll( colours::patternPickerBg );

			// Add/replace toggle button
			const auto toggleR = toggleRect();
			g.setColour( mAddMode ? colours::addBg : colours::replaceModeBg );
			g.fillRoundedRectangle( toggleR.toFloat(), BTN_CORNER_RADIUS );
			g.setColour( mAddMode ? colours::addAccent : colours::replaceModeAccent );
			g.drawRoundedRectangle( toggleR.toFloat(), BTN_CORNER_RADIUS, 1.0f );
			g.setColour( ::juce::Colours::white );
			g.setFont( FONT_SM );
			g.drawText( mAddMode ? "+ Add" : "Replace", toggleR, ::juce::Justification::centred );

			// Swatches
			const int count    = static_cast<int>( mPatterns.size() );
			const int swatchX0 = toggleRect().getRight() + GAP;
			const int swatchW  = count > 0 ? ( w - swatchX0 ) / count - GAP : 0;
			if ( swatchW < 10 )
				return;

			for ( int i = 0; i < count; ++i ) {
				const auto &p = mPatterns[ i ];
				const int   x = swatchX0 + i * ( swatchW + GAP );
				const auto  r = ::juce::Rectangle<int>{ x, 0, swatchW, h };

				// Background fill
				g.setColour( p.colour.withAlpha( mHoveredIndex == i ? 0.35f : 0.15f ) );
				g.fillRoundedRectangle( r.toFloat(), BTN_CORNER_RADIUS );

				// Border
				g.setColour( p.colour.withAlpha( mHoveredIndex == i ? 0.9f : 0.45f ) );
				g.drawRoundedRectangle( r.toFloat(), BTN_CORNER_RADIUS, 1.0f );

				// Mini rhythm preview — dots for each fire time in 0..4 beats
				const float     previewH = static_cast<float>( h ) * 0.38f;
				const float     previewY = BTN_CORNER_RADIUS;
				const float     previewX = static_cast<float>( x + 3 );
				const float     previewW = static_cast<float>( swatchW - 6 );
				constexpr float WIN      = 4.0f; // preview shows 4 beats

				// Find min/max pitch for y-mapping
				int minP = 127, maxP = 0;
				for ( const auto &n : p.notes ) {
					minP = std::min( minP, n.pitch );
					maxP = std::max( maxP, n.pitch );
				}
				const int pitchRange = std::max( 1, maxP - minP );

				for ( const auto &n : p.notes ) {
					if ( n.period <= 0.0f )
						continue;
					g.setColour( p.colour.withAlpha( 0.85f ) );
					for ( float beat = n.offset; beat < WIN; beat += n.period ) {
						const float nx = previewX + previewW * beat / WIN;
						const float ny = previewY + previewH * ( 1.0f - static_cast<float>( n.pitch - minP ) / pitchRange );
						g.fillRect( ::juce::Rectangle<float>{ nx - 1.0f, ny, 2.0f, previewH * 0.5f + 1.0f } );
					}
				}

				// Name label
				g.setColour( ::juce::Colours::white.withAlpha( 0.85f ) );
				g.setFont( FONT_SM );
				g.drawText( p.name, r.withTrimmedTop( h / 2 ), ::juce::Justification::centred );
			}
		}

		void mouseMove( const ::juce::MouseEvent &e ) override {
			mHoveredIndex = swatchIndexAt( e.getPosition() );
			repaint();
		}
		void mouseExit( const ::juce::MouseEvent & ) override {
			mHoveredIndex = -1;
			repaint();
		}

		void mouseDown( const ::juce::MouseEvent &e ) override {
			if ( toggleRect().contains( e.getPosition() ) ) {
				mAddMode = !mAddMode;
				repaint();
				return;
			}
			const int idx = swatchIndexAt( e.getPosition() );
			if ( idx >= 0 && mOnPickPattern )
				mOnPickPattern( mPatterns[ idx ].notes, mAddMode );
		}

	 private:
		static constexpr int TOGGLE_W = 52;
		static constexpr int GAP      = 3;

		const OnPickPattern     mOnPickPattern;
		std::vector<PatternDef> mPatterns;
		bool                    mAddMode      = false;
		int                     mHoveredIndex = -1;

		::juce::Rectangle<int> toggleRect() const {
			return { 0, 0, TOGGLE_W, getHeight() };
		}

		int swatchIndexAt( ::juce::Point<int> pos ) const {
			const int count    = static_cast<int>( mPatterns.size() );
			const int swatchX0 = toggleRect().getRight() + GAP;
			const int swatchW  = count > 0 ? ( getWidth() - swatchX0 ) / count - GAP : 0;
			if ( swatchW < 10 )
				return -1;
			for ( int i = 0; i < count; ++i ) {
				const int x = swatchX0 + i * ( swatchW + GAP );
				if ( ::juce::Rectangle<int>{ x, 0, swatchW, getHeight() }.contains( pos ) )
					return i;
			}
			return -1;
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( PatternPicker )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_PATTERN_PICKER_HPP
