#ifndef PLOP_SRC_UI_CC_CONTROLS_HPP
#define PLOP_SRC_UI_CC_CONTROLS_HPP

#include <array>
#include <cmath>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/midi.hpp"
#include "ui/colours.hpp"
#include "ui/displays/wave_thumbnail.hpp"
#include "ui/knob.hpp"
#include "ui/knob_cc_value.hpp"
#include "ui/knob_value_label.hpp"
#include "ui/knob_with_unit.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	/// Controls for the selected CC: Mute/Solo buttons, Period/Depth/Offset knobs,
	/// and a sin/tri/saw shape selector.
	class CcControls : public ::juce::Component {
	 public:
		struct Cbs {
			const ::std::function<void( float )> onChannelChanged;
			const ::std::function<void( float )> onOffsetChanged;
			const ::std::function<void( float )> onPeriodChanged;
			const ::std::function<void( float )> onCcChanged;
		};
		static constexpr int NUM_KNOBS = 3;

		CcControls( const Cbs &callbacks ) :
				  mPeriodKnob( "PERIOD", callbacks.onPeriodChanged ),
				  mChannelKnob( "CHANNEL",
		                      callbacks.onChannelChanged,
		                      []( const float &val ) { return ::juce::String( val + 1.0f, 0 ); } ),
				  mMidiKnob( "MIDI", callbacks.onCcChanged ),
				  mOffsetKnob( "OFFSET",
		                     callbacks.onOffsetChanged,
		                     []( const float &val ) { return ::juce::String( val, 2 ) + " b"; } ),
				  mCallbacks( callbacks ) {
			addAndMakeVisible( mPeriodKnob );
			addAndMakeVisible( mMidiKnob );
			addAndMakeVisible( mOffsetKnob );

			mChannelKnob.setRange( 0.0f, 15.0f );
			mPeriodKnob.setRange( 0.1f, 32.0f );
			mOffsetKnob.setRange( 0.0f, 32.0f );
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

			static constexpr WaveShape SHAPES[ 3 ] = { WaveShape::Sin, WaveShape::Tri, WaveShape::Saw };
			for ( int i = 0; i < 3; ++i ) {
				mWaveThumbs[ i ] = std::make_unique<WaveThumbnail>( SHAPES[ i ], [ this, s = SHAPES[ i ] ] {
					mShape = s;
					updateThumbSelection();
					if ( mOnShapeChanged )
						mOnShapeChanged( mShape );
				} );
				addAndMakeVisible( *mWaveThumbs[ i ] );
			}
			updateThumbSelection();

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
			updateThumbSelection();
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
		}

		void resized() override {
			const int thirdW = getWidth() / 3;
			const int knobY  = BTN_ROW_H;
			const int knobH  = getHeight() - BTN_ROW_H - SHAPE_ROW_H;
			const int shapeY = BTN_ROW_H + knobH + PAD_LG;
			const int thumbW = getWidth() / 4;

			mBtnMute.setBounds( 0, 0, thirdW, BTN_ROW_H );
			mBtnSolo.setBounds( thirdW, 0, thirdW, BTN_ROW_H );
			mBtnDelete.setBounds( thirdW * 2, 0, getWidth() - thirdW * 2, BTN_ROW_H );

			for ( int i = 0; i < 3; ++i ) {
				const int x = static_cast<int>( ( 0.5 + i ) * ( thumbW + PAD_MD ) );
				mWaveThumbs[ i ]->setBounds( x, shapeY, thumbW, SHAPE_ROW_H );
			}

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

	 private:
		static constexpr int BTN_ROW_H   = 26;
		static constexpr int SHAPE_ROW_H = WaveThumbnail::LABEL_H + 32; // waveform area + label

		void updateThumbSelection() {
			static constexpr WaveShape SHAPES[ 3 ] = { WaveShape::Sin, WaveShape::Tri, WaveShape::Saw };
			for ( int i = 0; i < 3; ++i )
				if ( mWaveThumbs[ i ] )
					mWaveThumbs[ i ]->setSelected( SHAPES[ i ] == mShape );
		}

		KnobWithUnit                                  mPeriodKnob;
		KnobValueLabel                                mChannelKnob;
		KnobCcValue                                   mMidiKnob;
		KnobValueLabel                                mOffsetKnob;
		std::array<std::unique_ptr<WaveThumbnail>, 3> mWaveThumbs;
		::juce::TextButton                            mBtnMute{ "Mute" };
		::juce::TextButton                            mBtnSolo{ "Solo" };
		::juce::TextButton                            mBtnDelete{ "Delete" };

		WaveShape mShape = WaveShape::Sin;

		bool                             mMuted      = false;
		bool                             mSoloed     = false;
		bool                             mStandalone = false;
		Cbs                              mCallbacks;
		std::function<void( WaveShape )> mOnShapeChanged;
		std::function<void( bool )>      mOnMuteChanged;
		std::function<void( bool )>      mOnSoloChanged;
		std::function<void()>            mOnDelete;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( CcControls )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_CC_CONTROLS_HPP
