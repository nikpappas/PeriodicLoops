#ifndef PLOP_SRC_UI_ORBITAL_DISPLAY_HPP
#define PLOP_SRC_UI_ORBITAL_DISPLAY_HPP

#include <cmath>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

namespace plop::ui {

	class OrbitalDisplay : public ::juce::Component {
	 public:
		struct VoiceState {
			float phase;     // [0, 1) — position around the circle
			bool  triggered; // true on the frame a lap completes
		};

		void setVoices( std::vector<VoiceState> voices ) {
			m_flash.resize( voices.size(), 0 );
			for ( int i = 0; i < static_cast<int>( voices.size() ); ++i ) {
				if ( voices[ i ].triggered ) {
					m_flash[ i ] = k_flash_frames;
				}
			}
			m_voices = std::move( voices );
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( ::juce::Colour( 0xff1a1a2e ) );

			if ( m_voices.empty() )
				return;

			const int   n     = static_cast<int>( m_voices.size() );
			const float cellW = static_cast<float>( getWidth() );
			const float cellH = static_cast<float>( getHeight() );

			for ( int i = 0; i < n; ++i ) {
				const auto &voice = m_voices[ i ];

				const float cx     = cellW * 0.5f;
				const float cy     = cellH * 0.5f;
				const float radius = std::min( cellW, cellH ) * 0.35f;

				// Dot — travels clockwise from 12 o'clock
				const float angle = voice.phase * ::juce::MathConstants<float>::twoPi;
				const float dotX  = cx + radius * std::sin( angle );
				const float dotY  = cy - radius * std::cos( angle );
				const float t     = static_cast<float>( m_flash[ i ] ) / k_flash_frames;
				const float dotR  = k_dot_r_normal + ( k_dot_r_peak - k_dot_r_normal ) * t;
				g.setColour( ::juce::Colour( 0xffaaaacc ) );
				g.fillEllipse( dotX - dotR, dotY - dotR, dotR * 2.0f, dotR * 2.0f );

				if ( m_flash[ i ] > 0 )
					--m_flash[ i ];
			}
		}

	 private:
		static constexpr int   k_flash_frames = 5;    // ~167 ms at 30 Hz
		static constexpr float k_dot_r_normal = 5.0f;
		static constexpr float k_dot_r_peak   = 14.0f;

		std::vector<VoiceState> m_voices;
		std::vector<int>        m_flash;
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_ORBITAL_DISPLAY_HPP
