#ifndef PLOP_SRC_UI_DISPLAYS_WAVE_THUMBNAIL_HPP
#define PLOP_SRC_UI_DISPLAYS_WAVE_THUMBNAIL_HPP

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/midi.hpp"
#include "ui/colours.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	/// A clickable waveform thumbnail for one WaveShape.
	/// Highlighted when selected. Calls onShapeChanged on click.
	class WaveThumbnail : public ::juce::Component {
	 public:
		static constexpr int LABEL_H = static_cast<int>( FONT_SM ) + 4;

		WaveThumbnail( WaveShape shape, std::function<void()> onShapeChanged ) :
				  mShape( shape ), mOnShapeChanged( std::move( onShapeChanged ) ) {
		}

		void setSelected( bool selected ) {
			if ( mIsSelected == selected )
				return;
			mIsSelected = selected;
			repaint();
		}

		void paint( ::juce::Graphics &g ) override {
			const auto thumbBounds = getLocalBounds().withTrimmedBottom( LABEL_H );

			g.setColour( mIsSelected ? colours::darkestGrey : colours::lightestGrey );
			g.fillRoundedRectangle( thumbBounds.toFloat(), 8.0f );

			const float bx     = static_cast<float>( thumbBounds.getX() );
			const float by     = static_cast<float>( thumbBounds.getY() );
			const float bh     = static_cast<float>( thumbBounds.getHeight() );
			const float margin = 3.0f;
			const float waveH  = bh - 2.0f * margin;
			const int   steps  = thumbBounds.getWidth();

			::juce::Path wave;
			for ( int px = 0; px < steps; ++px ) {
				const float t   = static_cast<float>( px ) / static_cast<float>( steps );
				const float val = evalWaveShape( mShape, t );
				const float py  = by + margin + waveH * ( 1.0f - val );
				if ( px == 0 )
					wave.startNewSubPath( bx + static_cast<float>( px ), py );
				else
					wave.lineTo( bx + static_cast<float>( px ), py );
			}
			g.setColour( mIsSelected ? colours::lightestGrey : colours::darkestGrey.withAlpha( 0.6f ) );
			g.strokePath( wave, ::juce::PathStrokeType( 1.2f ) );

			g.setColour( mIsSelected ? colours::accentOrange : colours::darkestGrey );
			g.setFont( ::juce::Font( FONT_SM ) );
			g.drawText(
			  ::plop::utils::to_string( mShape ), 0, getHeight() - LABEL_H, getWidth(), LABEL_H, ::juce::Justification::centred );
		}

		void mouseDown( const ::juce::MouseEvent & ) override {
			if ( mOnShapeChanged )
				mOnShapeChanged();
		}

	 private:
		WaveShape             mShape;
		bool                  mIsSelected = false;
		std::function<void()> mOnShapeChanged;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( WaveThumbnail )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_DISPLAYS_WAVE_THUMBNAIL_HPP
