#ifndef PLOP_SRC_UI_ORBITAL_DISPLAY_HPP
#define PLOP_SRC_UI_ORBITAL_DISPLAY_HPP

#include <cmath>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ui/colours.hpp"

namespace plop::ui {

	class OrbitalDisplay : public ::juce::Component {
	 public:
		OrbitalDisplay() {
		}
		struct VoiceState {
			float phase;       // [0, 1) — position around the circle
			bool  triggered;   // true on the frame a lap completes
			float trackRadius; // normalised [0, 1] — maps to pixel radius in paint()
		};

		void setVoices( const std::vector<VoiceState> &voices ) {
			if ( voices.size() != mVoices.size() )
				mTrigger.assign( voices.size(), 0 );
			for ( int i = 0; i < static_cast<int>( voices.size() ); ++i ) {
				if ( voices[ i ].triggered )
					mTrigger[ i ] = FLASH_FRAMES;
			}
			mVoices = voices;
		}

		void setColours( const ::std::vector<::juce::Colour> &colors ) {
			mColours = colors;
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( colours::orbitalBg );
			if ( mVoices.empty() )
				return;

			const float cx   = getWidth() * 0.5f;
			const float cy   = getHeight() * 0.5f;
			const float maxR = std::min( cx, cy ) * 0.9f;
			const float minR = maxR * MIN_RADIUS_RATIO;

			g.setColour( colours::inputBg );

			// Draw dots
			for ( int i = 0; i < static_cast<int>( mVoices.size() ); ++i ) {
				const auto &voice  = mVoices[ i ];
				const float r      = minR + voice.trackRadius * ( maxR - minR );
				const float angle  = voice.phase * ::juce::MathConstants<float>::twoPi;
				const float dotX   = cx + r * std::sin( angle );
				const float dotY   = cy - r * std::cos( angle );
				const float t      = static_cast<float>( mTrigger[ i ] ) / FLASH_FRAMES;
				const float dotR   = DOT_R_NORMAL + ( DOT_R_PEAK - DOT_R_NORMAL ) * t;
				const auto  colour = i < static_cast<int>( mColours.size() ) ? mColours[ i ] : colours::orbitalDotFallback;

				g.setColour( colour );
				g.fillEllipse( dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f );

				if ( mTrigger[ i ] > 0 )
					--mTrigger[ i ];
			}
		}

	 private:
		static constexpr int   FLASH_FRAMES      = 10;
		static constexpr float DOT_R_NORMAL      = 5.0f;
		static constexpr float DOT_R_PEAK        = 14.0f;
		static constexpr float MIN_RADIUS_RATIO  = 0.15f; // innermost ring as fraction of maxR

		::std::vector<VoiceState>     mVoices;
		::std::vector<::juce::Colour> mColours;
		::std::vector<int>            mTrigger;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( OrbitalDisplay )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_ORBITAL_DISPLAY_HPP
