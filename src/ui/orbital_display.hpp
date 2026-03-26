#ifndef PLOP_SRC_UI_ORBITAL_DISPLAY_HPP
#define PLOP_SRC_UI_ORBITAL_DISPLAY_HPP

#include <cmath>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

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

		void setVoices( std::vector<VoiceState> voices ) {
			if ( voices.size() != mVoices.size() )
				mTrigger.assign( voices.size(), 0 );
			for ( int i = 0; i < static_cast<int>( voices.size() ); ++i ) {
				if ( voices[ i ].triggered )
					mTrigger[ i ] = k_flash_frames;
			}
			mVoices = std::move( voices );
		}

		void setColours( const ::std::vector<::juce::Colour> &colors ) {
			mColours = colors;
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( ::juce::Colour( 0xff1a1a2e ) );
			if ( mVoices.empty() )
				return;

			const float cx   = getWidth() * 0.5f;
			const float cy   = getHeight() * 0.5f;
			const float maxR = std::min( cx, cy ) * 0.9f;
			const float minR = maxR * k_min_radius_ratio;

			g.setColour( ::juce::Colour( 0xff2a2a44 ) );

			// Draw dots
			for ( int i = 0; i < static_cast<int>( mVoices.size() ); ++i ) {
				const auto &voice  = mVoices[ i ];
				const float r      = minR + voice.trackRadius * ( maxR - minR );
				const float angle  = voice.phase * ::juce::MathConstants<float>::twoPi;
				const float dotX   = cx + r * std::sin( angle );
				const float dotY   = cy - r * std::cos( angle );
				const float t      = static_cast<float>( mTrigger[ i ] ) / k_flash_frames;
				const float dotR   = k_dot_r_normal + ( k_dot_r_peak - k_dot_r_normal ) * t;
				const auto  colour = i < static_cast<int>( mColours.size() ) ? mColours[ i ] : ::juce::Colour( 0xffaaaacc );

				g.setColour( colour );
				g.fillEllipse( dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f );

				if ( mTrigger[ i ] > 0 )
					--mTrigger[ i ];
			}
		}

	 private:
		static constexpr int   k_flash_frames     = 10;
		static constexpr float k_dot_r_normal     = 5.0f;
		static constexpr float k_dot_r_peak       = 14.0f;
		static constexpr float k_min_radius_ratio = 0.15f; // innermost ring as fraction of maxR

		::std::vector<VoiceState>     mVoices;
		::std::vector<::juce::Colour> mColours;
		::std::vector<int>            mTrigger;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( OrbitalDisplay )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_ORBITAL_DISPLAY_HPP
