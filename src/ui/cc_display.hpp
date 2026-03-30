#ifndef PLOP_SRC_UI_CC_DISPLAY_HPP
#define PLOP_SRC_UI_CC_DISPLAY_HPP

#include <cmath>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/midi.hpp"
#include "ui/colours.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	namespace {
		inline ::juce::String ccDisplayName( int cc ) {
			switch ( cc ) {
				case 0:
					return "Bank Sel";
				case 1:
					return "Mod";
				case 2:
					return "Breath";
				case 4:
					return "Foot";
				case 5:
					return "Port.T";
				case 7:
					return "Volume";
				case 8:
					return "Balance";
				case 10:
					return "Pan";
				case 11:
					return "Expr";
				case 64:
					return "Sustain";
				case 71:
					return "Resonance";
				case 72:
					return "Rel.Time";
				case 73:
					return "Atk.Time";
				case 74:
					return "Brightness";
				case 91:
					return "Reverb";
				case 93:
					return "Chorus";
				default:
					return "CC " + ::juce::String( cc );
			}
		}
	} // namespace

	/// Draws a scrolling waveform lane for each active CC.
	/// Push state each timer tick via setCCs() + setCurrentBeat() then repaint().
	class CcDisplay : public ::juce::Component {
	 public:
		static constexpr int LANE_H  = 42;
		static constexpr int LABEL_W = 62;

		CcDisplay() {
		}
		int getPreferredHeight() const {
			return ::juce::jmax( LANE_H, static_cast<int>( mCcs.size() ) * LANE_H );
		}

		void setCCs( const std::vector<PeriodicCC> &ccs ) {
			mCcs = std::move( ccs );
		}
		void setCurrentBeat( float beat ) {
			mCurrentBeat = beat;
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( colours::lcdBgColour );

			if ( mCcs.empty() ) {
				g.setColour( ::juce::Colours::grey.withAlpha( 0.3f ) );
				g.setFont( FONT_MD );
				g.drawText( "No CC events", getLocalBounds(), ::juce::Justification::centred );
				return;
			}

			// How many beats the full width represents; cursor sits at 25 % from left
			constexpr float WINDOW    = 8.0f;
			constexpr float CURSOR_T  = 0.25f;
			const float     beatStart = mCurrentBeat - WINDOW * CURSOR_T;
			const int       w         = getWidth();
			auto            isAnySolo = false;
			for ( int i = 0; i < static_cast<int>( mCcs.size() ); ++i ) {
				if ( mCcs[ i ].solo ) {
					isAnySolo = true;
					break;
				}
			}

			for ( int i = 0; i < static_cast<int>( mCcs.size() ); ++i ) {
				const auto &cc         = mCcs[ i ];
				const auto  isDisabled = isAnySolo && !cc.solo;
				const int   y          = i * LANE_H;

				// Lane background
				g.setColour( i % 2 == 0 ? colours::panelBg : colours::lcdBgColour );
				g.fillRect( 0, y, w, LANE_H );

				if ( cc.period <= 0.0f )
					continue;

				// Beat grid lines (subtle)
				g.setColour( colours::subtleGrey );
				for ( float b = std::ceil( beatStart ); b < beatStart + WINDOW; b += 1.0f ) {
					const int gx = static_cast<int>( ( b - beatStart ) / WINDOW * w );
					g.drawVerticalLine( gx, static_cast<float>( y ), static_cast<float>( y + LANE_H ) );
				}

				// Waveform
				const int   margin = static_cast<int>( LANE_H * 0.12f );
				const float waveH  = static_cast<float>( LANE_H - 2 * margin );

				::juce::Path wave;
				for ( int px = 0; px < w; ++px ) {
					const float beat = beatStart + WINDOW * static_cast<float>( px ) / w;
					const float val  = 0.5f + 0.5f * std::sin( 2.0f * 3.14159265f * ( beat + cc.offset ) / cc.period );
					const float py   = y + margin + waveH * ( 1.0f - val );
					if ( px == 0 )
						wave.startNewSubPath( static_cast<float>( px ), py );
					else
						wave.lineTo( static_cast<float>( px ), py );
				}
				g.setColour( colours::accentBlue.withAlpha( isDisabled ? 0.25f : 0.75f ) );
				g.strokePath( wave, ::juce::PathStrokeType( 1.5f ) );

				// Cursor line
				const int cursorX = static_cast<int>( CURSOR_T * w );
				g.setColour( ::juce::Colours::white.withAlpha( 0.25f ) );
				g.drawVerticalLine( cursorX, static_cast<float>( y ), static_cast<float>( y + LANE_H ) );

				// Current-value dot on the waveform
				if ( !isDisabled ) {
					const float curVal = 0.5f + 0.5f * std::sin( 2.0f * 3.14159265f * ( mCurrentBeat + cc.offset ) / cc.period );
					const float dotY = y + margin + waveH * ( 1.0f - curVal );
					g.setColour( ::juce::Colours::white );
					g.fillEllipse( static_cast<float>( cursorX ) - 3.5f, dotY - 3.5f, 7.0f, 7.0f );
				}

				// Label (drawn over the left edge of the wave)
				g.setColour( colours::offWhite );
				g.setFont( FONT_SM );
				g.drawText( ccDisplayName( cc.number ), PAD_SM, y + ( LANE_H - 14 ) / 2, LABEL_W, 14, ::juce::Justification::centredLeft );

				// DisabledOverlay if cc is disabled
				if ( isDisabled ) {
					g.setColour( colours::subtleGrey.withAlpha( 0.6f ) );
					g.fillRect( 0, y, w, LANE_H );
				}

				// Separator
				g.setColour( colours::subtleGrey );
				g.drawHorizontalLine( y + LANE_H - 1, 0.0f, static_cast<float>( w ) );
			}
		}

	 private:
		std::vector<PeriodicCC> mCcs;
		float                   mCurrentBeat = 0.0f;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( CcDisplay )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_CC_DISPLAY_HPP
