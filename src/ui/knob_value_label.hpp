#ifndef PLOP_SRC_UI_KNOB_VALUE_LABEL_HPP
#define PLOP_SRC_UI_KNOB_VALUE_LABEL_HPP

#include <cmath>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ui/colours.hpp"
#include "ui/ui_constants.hpp"

#include "ui/knob.hpp"

namespace plop::ui {

	class KnobValueLabel : public ::juce::Component {
	 public:
		KnobValueLabel( const ::juce::String &label, std::function<void( float )> onChange ) : mKnob( label, onChange ) {
			addAndMakeVisible( mKnob );
		}

		void setLabel( const ::juce::String &label ) {
			mKnob.setLabel( label );
		}

		void setValue( float v ) {
			mKnob.setValue( v );
		}

		float getValue() const {
			return mKnob.getValue();
		}

		void setRange( float min, float max ) {
			mKnob.setRange( min, max );
		}

		void setActive( bool active ) {
			mKnob.setActive( active );
			repaint();
		}

		void resized() override {
			mKnob.setBounds( LEFT_W, EDGE_H, getWidth() - LEFT_W, getHeight() - 2 * EDGE_H );
		}
		void paint( ::juce::Graphics &g ) override {

			if ( mKnob.isActive() ) {

				g.setColour( colours::accentBlue );

				// --- Label ---
				g.setColour( ::juce::Colours::black );
				g.setFont( ::juce::Font( FONT_SM ) );

				g.drawText( ::juce::String( mKnob.getValue() + 1, 0 ),
				            LEFT_W,
				            getHeight() - EDGE_H,
				            getWidth() - LEFT_W,
				            EDGE_H,
				            ::juce::Justification::centred );
			}
		}

	 private:
		static constexpr int LEFT_W = 40;
		static constexpr int EDGE_H = static_cast<int>( FONT_SM ) + 6;

		Knob mKnob;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( KnobValueLabel )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_KNOB_VALUE_LABEL_HPP
