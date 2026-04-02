#ifndef PLOP_SRC_UI_KNOB_HPP
#define PLOP_SRC_UI_KNOB_HPP

#include <cmath>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ui/colours.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	/// A minimal rotary knob — thin circle outline + single indicator line + label.
	/// Drag up/down to change value. 0° = bottom-left (224°), full range = 270° clockwise sweep.
	class Knob : public ::juce::Component {
	 public:
		Knob( const ::juce::String &label, const std::function<void( float )> onChange ) :
				  mLabel( label ), mOnChange( std::move( onChange ) ) {
			setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
		}

		void setLabel( const ::juce::String &label ) {
			mLabel = label;
			repaint();
		}

		void setOnChange( std::function<void( float )> onChange ) {
			mOnChange = std::move( onChange );
		}

		void setValue( float v ) {
			mValue = ::juce::jlimit( mMin, mMax, v );
			repaint();
		}

		float getValue() const {
			return mValue;
		}

		float getNormValue() const {
			return ( mMax > mMin ) ? ( mValue - mMin ) / ( mMax - mMin ) : 0.0f;
		}

		void setRange( float min, float max ) {
			mMin   = min;
			mMax   = max;
			mValue = ::juce::jlimit( mMin, mMax, mValue );
			repaint();
		}

		void setActive( bool active ) {
			mActive = active;
			repaint();
		}

		bool isActive() const {
			return mActive;
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( colours::accentBlue );
			const float labelH = mLabel.isEmpty() ? 0.0f : FONT_SM + 6.0f;

			g.setColour( ::juce::Colours::black );
			g.setFont( ::juce::Font( FONT_SM ) );
			g.drawText( mLabel, 0, 0, getWidth(), static_cast<int>( labelH ), ::juce::Justification::centred );

			const float dialH = static_cast<float>( getHeight() ) - labelH;
			const float size  = std::min( static_cast<float>( getWidth() ), dialH );
			const float cx    = static_cast<float>( getWidth() ) / 2.0f;
			const float cy    = dialH / 2.0f + 16.0f;
			const float r     = size / 2.0f - 4.0f;

			// --- Circle ---
			g.setColour( colours::darkestGrey );
			g.drawEllipse( cx - r, cy - r, r * 2.0f, r * 2.0f, 1.5f );

			// --- Indicator line ---
			if ( mActive ) {
				const float norm  = getNormValue();
				const float angle = START_ANGLE + ( END_ANGLE - START_ANGLE ) * norm;
				const float lx    = cx + r * std::sin( angle );
				const float ly    = cy - r * std::cos( angle );
				g.setColour( colours::accentOrange );
				g.drawLine( cx, cy, lx, ly, 1.5f );
			}

			// --- Label ---
		}

		void mouseDown( const ::juce::MouseEvent &e ) override {
			mDragStartY     = e.getPosition().y;
			mDragStartValue = mValue;
		}

		void mouseDrag( const ::juce::MouseEvent &e ) override {
			const int   dy       = mDragStartY - e.getPosition().y;
			const float range    = mMax - mMin;
			const float newValue = ::juce::jlimit( mMin, mMax, mDragStartValue + dy * range / DRAG_SENSITIVITY );
			if ( newValue == mValue )
				return;
			mValue = newValue;
			if ( mOnChange )
				mOnChange( mValue );
			repaint();
		}

	 private:
		// 270° clockwise sweep starting at 224° from top (≈ 7:30 position)
		static constexpr float START_ANGLE      = 224.0f * ::juce::MathConstants<float>::pi / 180.0f;
		static constexpr float END_ANGLE        = START_ANGLE + 270.0f * ::juce::MathConstants<float>::pi / 180.0f;
		static constexpr float DRAG_SENSITIVITY = 150.0f; // px for full range

		::juce::String               mLabel;
		std::function<void( float )> mOnChange;
		float                        mMin            = 0.0f;
		float                        mMax            = 1.0f;
		float                        mValue          = 0.0f;
		int                          mDragStartY     = 0;
		float                        mDragStartValue = 0.0f;
		bool                         mActive         = true;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( Knob )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_KNOB_HPP
