#ifndef PLOP_SRC_UI_CC_CONTROLS_HPP
#define PLOP_SRC_UI_CC_CONTROLS_HPP

#include <array>
#include <cmath>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/midi.hpp"
#include "ui/colours.hpp"
#include "ui/knob.hpp"
#include "ui/knob_value_label.hpp"
#include "ui/knob_with_unit.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	/// Controls for the selected CC: Mute/Solo buttons, Period/Depth/Offset knobs,
	/// and a sin/tri/saw shape selector.
	class CcControls : public ::juce::Component {
	 public:
		struct Cbs {
			const ::std::function<void( int )> onChannelChanged;
		};
		static constexpr int NUM_KNOBS = 3;

		CcControls( const Cbs &callbacks ) :
				  mPeriodKnob( "PERIOD", {} ),
				  mChannelKnob( "CHANNEL", callbacks.onChannelChanged ),
				  mMidiKnob( "MIDI", {} ),
				  mOffsetKnob( "OFFSET", {} ),
				  mCallbacks( callbacks ) {
			static constexpr const char *LABELS[ 2 ] = { "MIDI", "OFFSET" };
			// for ( int i = 0; i < 2; ++i ) {
			// 	mKnobs[ static_cast<size_t>( i ) ] = std::make_unique<Knob>( { "", {} } );
			// 	mKnobs[ static_cast<size_t>( i ) ]->setLabel( LABELS[ i ] );
			// 	addAndMakeVisible( *mKnobs[ static_cast<size_t>( i ) ] );
			// }
			addAndMakeVisible( mPeriodKnob );
			addAndMakeVisible( mMidiKnob );
			addAndMakeVisible( mOffsetKnob );

			mChannelKnob.setRange( 0.0f, 15.0f );
			addChildComponent( mChannelKnob );

			for ( auto *btn : { &mBtnMute, &mBtnSolo } ) {
				btn->setClickingTogglesState( false );
				btn->setColour( ::juce::TextButton::buttonOnColourId, colours::btnAccentColour );
				addAndMakeVisible( *btn );
			}
			mBtnDelete.setClickingTogglesState( false );
			mBtnDelete.setColour( ::juce::TextButton::buttonColourId, colours::removeBg );
			mBtnDelete.setColour( ::juce::TextButton::textColourOffId, colours::removeAccent );
			addAndMakeVisible( mBtnDelete );

			mBtnMute.onClick = [ this ] {
				mMuted = !mMuted;
				mBtnMute.setToggleState( mMuted, ::juce::dontSendNotification );
				if ( mOnMuteChanged )
					mOnMuteChanged( mMuted );
			};
			mBtnSolo.onClick = [ this ] {
				mSoloed = !mSoloed;
				mBtnSolo.setToggleState( mSoloed, ::juce::dontSendNotification );
				if ( mOnSoloChanged )
					mOnSoloChanged( mSoloed );
			};
			mBtnDelete.onClick = [ this ] {
				if ( mOnDelete )
					mOnDelete();
			};
		}

		void setOnMuteChanged( std::function<void( bool )> cb ) {
			mOnMuteChanged = std::move( cb );
		}
		void setOnSoloChanged( std::function<void( bool )> cb ) {
			mOnSoloChanged = std::move( cb );
		}
		void setOnDelete( std::function<void()> cb ) {
			mOnDelete = std::move( cb );
		}

		void configureKnob( int index, const ::juce::String &label, float min, float max, std::function<void( float )> onChange ) {
			switch ( index ) {
				case 0:
					mPeriodKnob.setLabel( label );
					mPeriodKnob.setRange( min, max );
					mPeriodKnob.setOnChange( std::move( onChange ) );
					return;
				case 1:
					mMidiKnob.setLabel( label );
					mMidiKnob.setRange( min, max );
					mMidiKnob.setOnChange( std::move( onChange ) );
					return;
				case 2:
					mOffsetKnob.setLabel( label );
					mOffsetKnob.setRange( min, max );
					mOffsetKnob.setOnChange( std::move( onChange ) );
					return;
				case 3:
					mChannelKnob.setLabel( label );
					mChannelKnob.setRange( min, max );
					return;
			}
		}

		void setOnShapeChanged( std::function<void( WaveShape )> cb ) {
			mOnShapeChanged = std::move( cb );
		}

		void setKnobValue( int index, float value ) {
			switch ( index ) {
				case 0:
					mPeriodKnob.setValue( value );
					return;
				case 1:
					mMidiKnob.setValue( value );
					return;
				case 2:
					mOffsetKnob.setValue( value );
					return;
				case 3:
					mChannelKnob.setValue( value );
					return;
			}
		}

		float getKnobValue( int index ) const {
			switch ( index ) {
				case 0:
					return mPeriodKnob.getValue();
				case 1:
					return mMidiKnob.getValue();
				case 2:
					return mOffsetKnob.getValue();
				case 3:
					return mChannelKnob.getValue();
			}

			return 0.0f;
		}

		void setShape( WaveShape shape ) {
			mShape = shape;
			repaint();
		}

		void setMuted( bool muted ) {
			mMuted = muted;
			mBtnMute.setToggleState( muted, ::juce::dontSendNotification );
		}

		void setSoloed( bool soloed ) {
			mSoloed = soloed;
			mBtnSolo.setToggleState( soloed, ::juce::dontSendNotification );
		}

		void setHasSelection( bool hasSelection ) {
			mPeriodKnob.setActive( hasSelection );
			mMidiKnob.setActive( hasSelection );
			mOffsetKnob.setActive( hasSelection );
			mChannelKnob.setActive( hasSelection );
		}

		void setStandalone( bool standalone ) {
			mStandalone = standalone;
			mChannelKnob.setVisible( standalone );
			resized();
		}

		void setChannelValue( int channel ) {
			mChannelKnob.setValue( static_cast<float>( channel ) );
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( colours::panelBg );
			g.setColour( colours::borderLine );
			g.drawHorizontalLine( 0, 0.0f, static_cast<float>( getWidth() ) );

			// Shape selector row at bottom
			const int rowY = getHeight() - SHAPE_ROW_H;
			g.setColour( colours::darkestGrey );
			g.setFont( ::juce::Font( FONT_SM ) );
			g.drawText( "shape", PAD_MD, rowY, LABEL_W, SHAPE_ROW_H, ::juce::Justification::centredLeft );

			const int optW = ( getWidth() - PAD_MD - LABEL_W ) / 3;
			const int optX = PAD_MD + LABEL_W;
			drawShapeOption( g, optX, rowY, optW, "sin", mShape == WaveShape::Sin );
			drawShapeOption( g, optX + optW, rowY, optW, "tri", mShape == WaveShape::Tri );
			drawShapeOption( g, optX + optW * 2, rowY, optW, "saw", mShape == WaveShape::Saw );
		}

		void resized() override {
			const int thirdW = getWidth() / 3;
			const int knobY  = BTN_ROW_H;
			const int knobH  = getHeight() - BTN_ROW_H - SHAPE_ROW_H;

			mBtnMute.setBounds( 0, 0, thirdW, BTN_ROW_H );
			mBtnSolo.setBounds( thirdW, 0, thirdW, BTN_ROW_H );
			mBtnDelete.setBounds( thirdW * 2, 0, getWidth() - thirdW * 2, BTN_ROW_H );

			if ( mStandalone ) {
				const int colW = getWidth() / 3;
				mPeriodKnob.setBounds( 0, knobY, colW, knobH );
				mMidiKnob.setBounds( colW, knobY, colW, knobH / 2 );
				mOffsetKnob.setBounds( colW, knobY + knobH / 2, colW, knobH - knobH / 2 );
				mChannelKnob.setBounds( colW * 2, knobY, getWidth() - colW * 2, knobH );
			} else {
				const int halfW = getWidth() / 2;
				mPeriodKnob.setBounds( 0, knobY, halfW, knobH );

				mMidiKnob.setBounds( halfW, knobY, getWidth() - halfW, knobH / 2 );
				mOffsetKnob.setBounds( halfW, knobY + knobH / 2, getWidth() - halfW, knobH - knobH / 2 );
			}
		}

		void mouseDown( const ::juce::MouseEvent &e ) override {
			const int rowY = getHeight() - SHAPE_ROW_H;
			if ( e.y < rowY )
				return;

			const int optW = ( getWidth() - PAD_MD - LABEL_W ) / 3;
			const int relX = e.x - PAD_MD - LABEL_W;
			if ( relX < 0 )
				return;

			const int idx = relX / optW;
			WaveShape newShape;
			switch ( idx ) {
				case 0:
					newShape = WaveShape::Sin;
					break;
				case 1:
					newShape = WaveShape::Tri;
					break;
				case 2:
					newShape = WaveShape::Saw;
					break;
				default:
					return;
			}
			mShape = newShape;
			repaint();
			if ( mOnShapeChanged )
				mOnShapeChanged( mShape );
		}

	 private:
		static constexpr int   BTN_ROW_H   = 26;
		static constexpr int   SHAPE_ROW_H = static_cast<int>( FONT_SM ) + 8;
		static constexpr int   LABEL_W     = 36;
		static constexpr float DOT_R       = 3.5f;

		static void drawShapeOption( ::juce::Graphics &g, int x, int y, int w, const ::juce::String &label, bool selected ) {
			const float cy   = static_cast<float>( y ) + static_cast<float>( SHAPE_ROW_H ) / 2.0f;
			const float dotX = static_cast<float>( x ) + DOT_R + 3.0f;

			g.setColour( colours::darkestGrey );
			g.drawEllipse( dotX - DOT_R, cy - DOT_R, DOT_R * 2.0f, DOT_R * 2.0f, 1.0f );
			if ( selected )
				g.fillEllipse( dotX - DOT_R + 1.5f, cy - DOT_R + 1.5f, ( DOT_R - 1.5f ) * 2.0f, ( DOT_R - 1.5f ) * 2.0f );

			g.setFont( ::juce::Font( FONT_SM ) );
			const int textX = static_cast<int>( dotX + DOT_R + 3.0f );
			g.drawText( label, textX, y, x + w - textX, SHAPE_ROW_H, ::juce::Justification::centredLeft );
		}

		KnobWithUnit                     mPeriodKnob;
		KnobValueLabel                   mChannelKnob;
		Knob                             mMidiKnob;
		Knob                             mOffsetKnob;
		::juce::TextButton               mBtnMute{ "Mute" };
		::juce::TextButton               mBtnSolo{ "Solo" };
		::juce::TextButton               mBtnDelete{ "Delete" };
		WaveShape                        mShape      = WaveShape::Sin;
		bool                             mMuted      = false;
		bool                             mSoloed     = false;
		bool                             mStandalone = false;
		Cbs                              mCallbacks;
		std::function<void( WaveShape )> mOnShapeChanged;
		std::function<void( bool )>      mOnMuteChanged;
		std::function<void( bool )>      mOnSoloChanged;
		std::function<void()>            mOnDelete;
		std::function<void( int )>       mOnChannelChanged;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( CcControls )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_CC_CONTROLS_HPP
