#ifndef PLOP_SRC_UI_NOTE_CONTROLS_HPP
#define PLOP_SRC_UI_NOTE_CONTROLS_HPP

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/midi.hpp"
#include "ui/colours.hpp"
#include "ui/knob_value_label.hpp"
#include "ui/knob_with_unit.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	namespace {
		inline ::juce::String noteName( int pitch ) {
			static constexpr const char *names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
			return ::juce::String( names[ pitch % 12 ] ) + ::juce::String( pitch / 12 - 2 )
			       + " (" + ::juce::String( pitch ) + ")";
		}
	} // namespace

	/// Controls for the selected note: Pitch / Period / Offset / Duration knobs and a Delete button.
	/// Mirrors CcControls in structure; no mute/solo since PeriodicNote has no such flags.
	class NoteControls : public ::juce::Component {
	 public:
		struct Cbs {
			std::function<void( float )> onPitchChanged;
			std::function<void( float )> onPeriodChanged;
			std::function<void( float )> onOffsetChanged;
			std::function<void( float )> onDurationChanged;
			std::function<void( float )> onChannelChanged;
		};

		explicit NoteControls( const Cbs &cbs ) :
				  mPitchKnob( "PITCH",
				              cbs.onPitchChanged,
				              []( const float &v ) { return noteName( static_cast<int>( v ) ); } ),
				  mPeriodKnob( "PERIOD", cbs.onPeriodChanged ),
				  mOffsetKnob( "OFFSET",
				               cbs.onOffsetChanged,
				               []( const float &v ) { return ::juce::String( v, 2 ) + " b"; } ),
				  mDurationKnob( "DURATION",
				                 cbs.onDurationChanged,
				                 []( const float &v ) { return ::juce::String( v, 2 ) + " b"; } ),
				  mChannelKnob( "CHANNEL",
				                cbs.onChannelChanged,
				                []( const float &v ) { return ::juce::String( static_cast<int>( v ) + 1 ); } ) {
			mPitchKnob.setRange( 0.0f, 127.0f );
			mPeriodKnob.setRange( 0.1f, 32.0f );
			mOffsetKnob.setRange( 0.0f, 32.0f );
			mDurationKnob.setRange( 0.01f, 32.0f );
			mChannelKnob.setRange( 0.0f, 15.0f );

			addAndMakeVisible( mPitchKnob );
			addAndMakeVisible( mPeriodKnob );
			addAndMakeVisible( mOffsetKnob );
			addAndMakeVisible( mDurationKnob );
			addChildComponent( mChannelKnob );

			mBtnDelete.setClickingTogglesState( false );
			mBtnDelete.setColour( ::juce::TextButton::buttonColourId, colours::removeBg );
			mBtnDelete.setColour( ::juce::TextButton::textColourOffId, colours::removeAccent );
			addAndMakeVisible( mBtnDelete );

			mBtnDelete.onClick = [ this ] {
				if ( mOnDelete )
					mOnDelete();
			};
		}

		void setOnDelete( std::function<void()> cb ) {
			mOnDelete = std::move( cb );
		}

		// index: 0=pitch, 1=period, 2=offset, 3=duration, 4=channel
		void setKnobValue( int index, float value ) {
			switch ( index ) {
				case 0: mPitchKnob.setValue( value ); return;
				case 1: mPeriodKnob.setValue( value ); return;
				case 2: mOffsetKnob.setValue( value ); return;
				case 3: mDurationKnob.setValue( value ); return;
				case 4: mChannelKnob.setValue( value ); return;
			}
		}

		float getKnobValue( int index ) const {
			switch ( index ) {
				case 0: return mPitchKnob.getValue();
				case 1: return mPeriodKnob.getValue();
				case 2: return mOffsetKnob.getValue();
				case 3: return mDurationKnob.getValue();
				case 4: return mChannelKnob.getValue();
			}
			return 0.0f;
		}

		void setHasSelection( bool has ) {
			mPitchKnob.setActive( has );
			mPeriodKnob.setActive( has );
			mOffsetKnob.setActive( has );
			mDurationKnob.setActive( has );
			mChannelKnob.setActive( has );
			mBtnDelete.setEnabled( has );
		}

		void setStandalone( bool standalone ) {
			mStandalone = standalone;
			mChannelKnob.setVisible( standalone );
			resized();
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( colours::panelBg );
			g.setColour( colours::borderLine );
			g.drawHorizontalLine( 0, 0.0f, static_cast<float>( getWidth() ) );
		}

		void resized() override {
			const int knobY = BTN_ROW_H;
			const int knobH = getHeight() - BTN_ROW_H;

			mBtnDelete.setBounds( 0, 0, getWidth(), BTN_ROW_H );

			if ( mStandalone ) {
				const int thirdW = getWidth() / 3;
				const int halfH  = knobH / 2;
				mPitchKnob.setBounds( 0, knobY, thirdW, halfH );
				mOffsetKnob.setBounds( 0, knobY + halfH, thirdW, knobH - halfH );
				mPeriodKnob.setBounds( thirdW, knobY, thirdW, halfH );
				mDurationKnob.setBounds( thirdW, knobY + halfH, thirdW, knobH - halfH );
				mChannelKnob.setBounds( thirdW * 2, knobY, getWidth() - thirdW * 2, knobH );
			} else {
				const int halfW = getWidth() / 2;
				const int halfH = knobH / 2;
				mPitchKnob.setBounds( 0, knobY, halfW, halfH );
				mOffsetKnob.setBounds( 0, knobY + halfH, halfW, knobH - halfH );
				mPeriodKnob.setBounds( halfW, knobY, halfW, halfH );
				mDurationKnob.setBounds( halfW, knobY + halfH, halfW, knobH - halfH );
			}
		}

	 private:
		static constexpr int BTN_ROW_H = 26;

		KnobValueLabel             mPitchKnob;
		KnobWithUnit               mPeriodKnob;
		KnobValueLabel             mOffsetKnob;
		KnobValueLabel             mDurationKnob;
		KnobValueLabel             mChannelKnob;
		::juce::TextButton         mBtnDelete{ "Delete" };
		std::function<void()>      mOnDelete;
		bool                       mStandalone = false;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( NoteControls )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_NOTE_CONTROLS_HPP
