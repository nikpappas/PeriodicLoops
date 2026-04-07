#ifndef PLOP_SRC_UI_KNOB_WITH_UNIT_HPP
#define PLOP_SRC_UI_KNOB_WITH_UNIT_HPP

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ui/colours.hpp"
#include "ui/knob.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	/// A Knob with a label at the top, a value readout at the bottom,
	/// and a sec / beats unit selector on the left.
	class KnobWithUnit : public ::juce::Component {
	 public:
		enum class Unit { Sec, Beats };

		KnobWithUnit( const ::juce::String &label, const std::function<void( float )> &onChange ) :
				  mKnob( label, onChange ) {
			addAndMakeVisible( mKnob );

			mEditor.setJustification( ::juce::Justification::centred );
			mEditor.setColour( ::juce::TextEditor::backgroundColourId, colours::inputBg );
			mEditor.setColour( ::juce::TextEditor::textColourId, ::juce::Colours::white );
			mEditor.setColour( ::juce::TextEditor::outlineColourId, colours::accentOrange );
			mEditor.onReturnKey = [ this ] { commitEdit(); };
			mEditor.onEscapeKey = [ this ] { cancelEdit(); };
			mEditor.onFocusLost = [ this ] { commitEdit(); };
			addChildComponent( mEditor );

			mKnob.onDoubleClicked = [ this ] { showEditor(); };
		}

		void setLabel( const ::juce::String &label ) {
			mKnob.setLabel( label );
		}

		void setOnChange( const std::function<void( float )> &onChange ) {
			mKnob.setOnChange( std::move( onChange ) );
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
		Unit getUnit() const {
			return mUnit;
		}

		void setActive( bool active ) {
			mActive = active;
			mKnob.setActive( active );
			repaint();
		}

		void setUnit( Unit u ) {
			mUnit = u;
			repaint();
		}

		void mouseDoubleClick( const ::juce::MouseEvent & ) override {
			showEditor();
		}

		void resized() override {
			mKnob.setBounds( LEFT_W, EDGE_H, getWidth() - LEFT_W, getHeight() - 2 * EDGE_H );
			mEditor.setBounds( LEFT_W, getHeight() - EDGE_H, getWidth() - LEFT_W, EDGE_H );
		}

		void paint( ::juce::Graphics &g ) override {

			// Value — bottom (only when a CC is selected)
			if ( mActive )
				g.drawText( ::juce::String( mKnob.getValue(), 2 ),
				            LEFT_W,
				            getHeight() - EDGE_H,
				            getWidth() - LEFT_W,
				            EDGE_H,
				            ::juce::Justification::centred );

			// Radio buttons — left, vertically centred
			const float h      = static_cast<float>( getHeight() );
			const float rowH   = static_cast<float>( EDGE_H );
			const float startY = ( h - rowH * 2.0f ) / 2.0f;
			drawRadio( g, static_cast<int>( startY ), static_cast<int>( rowH ), "sec", mUnit == Unit::Sec );
			drawRadio( g, static_cast<int>( startY + rowH ), static_cast<int>( rowH ), "beats", mUnit == Unit::Beats );
		}

		void mouseDown( const ::juce::MouseEvent &e ) override {
			if ( e.x >= LEFT_W )
				return;

			const float h      = static_cast<float>( getHeight() );
			const float rowH   = static_cast<float>( EDGE_H );
			const float startY = ( h - rowH * 2.0f ) / 2.0f;
			const int   y      = e.getPosition().y;

			if ( y >= static_cast<int>( startY ) && y < static_cast<int>( startY + rowH ) )
				setUnit( Unit::Sec );
			else if ( y >= static_cast<int>( startY + rowH ) && y < static_cast<int>( startY + rowH * 2.0f ) )
				setUnit( Unit::Beats );
		}

	 private:
		static constexpr int LEFT_W = 40;
		static constexpr int EDGE_H = static_cast<int>( FONT_SM ) + 6;

		void showEditor() {
			mEditor.setText( ::juce::String( mKnob.getValue(), 3 ), false );
			mEditor.setVisible( true );
			mEditor.grabKeyboardFocus();
			mEditor.selectAll();
		}
		void commitEdit() {
			const float v = mEditor.getText().getFloatValue();
			cancelEdit();
			mKnob.setValue( ::juce::jlimit( mKnob.getMin(), mKnob.getMax(), v ) );
			if ( mKnob.getOnChange() )
				mKnob.getOnChange()( mKnob.getValue() );
		}
		void cancelEdit() {
			mEditor.setVisible( false );
		}

		static constexpr float DOT_R      = 4.0f;
		static constexpr float DOT_OFFSET = 6.0f;

		void drawRadio( ::juce::Graphics &g, int y, int h, const ::juce::String &label, bool selected ) const {
			const float cy   = static_cast<float>( y ) + static_cast<float>( h ) / 2.0f;
			const float dotX = DOT_OFFSET;

			g.setColour( colours::darkestGrey );
			g.drawEllipse( dotX - DOT_R, cy - DOT_R, DOT_R * 2.0f, DOT_R * 2.0f, 1.0f );
			if ( selected )
				g.fillEllipse( dotX - DOT_R + 2.0f, cy - DOT_R + 2.0f, ( DOT_R - 2.0f ) * 2.0f, ( DOT_R - 2.0f ) * 2.0f );

			g.setFont( ::juce::Font( FONT_SM ) );
			const int textX = static_cast<int>( dotX + DOT_R + 3.0f );
			g.drawText( label, textX, y, LEFT_W - textX, h, ::juce::Justification::centredLeft );
		}

		Knob                   mKnob;
		::juce::TextEditor     mEditor;
		Unit                   mUnit   = Unit::Beats;
		bool                   mActive = true;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( KnobWithUnit )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_KNOB_WITH_UNIT_HPP
