#ifndef PLOP_SRC_UI_SETTINGS_PANEL_HPP
#define PLOP_SRC_UI_SETTINGS_PANEL_HPP

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/scales.hpp"
#include "processor/plugin_state.hpp"
#include "ui/colours.hpp"
#include "ui/note_list_panel.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	class SettingsPanel : public ::juce::Component {
	 public:
		struct Callbacks {
			std::function<void( PluginMode )> onModeChanged;
			std::function<void( float )>      onSilicaPeriodChanged;
			std::function<void( int, int )>   onScaleChanged; // root, typeIndex
		};

		explicit SettingsPanel( Callbacks cbs ) : mCbs( std::move( cbs ) ) {
			for ( auto *btn : { &mBtnPro, &mBtnMelody, &mBtnDrums, &mBtnSilica, &mBtnScale } ) {
				btn->setClickingTogglesState( false );
				btn->setColour( ::juce::TextButton::buttonOnColourId, colours::btnAccentColour );
				addAndMakeVisible( *btn );
			}

			mBtnPro.onClick    = [ this ] { fireMode( PluginMode::pro ); };
			mBtnMelody.onClick = [ this ] { fireMode( PluginMode::melody ); };
			mBtnDrums.onClick  = [ this ] { fireMode( PluginMode::drums ); };
			mBtnSilica.onClick = [ this ] { fireMode( PluginMode::silica ); };
			mBtnScale.onClick  = [ this ] { fireMode( PluginMode::scale ); };
		}

		void setMode( PluginMode mode ) {
			mMode = mode;
			mBtnPro.setToggleState( mode == PluginMode::pro, ::juce::dontSendNotification );
			mBtnMelody.setToggleState( mode == PluginMode::melody, ::juce::dontSendNotification );
			mBtnDrums.setToggleState( mode == PluginMode::drums, ::juce::dontSendNotification );
			mBtnSilica.setToggleState( mode == PluginMode::silica, ::juce::dontSendNotification );
			mBtnScale.setToggleState( mode == PluginMode::scale, ::juce::dontSendNotification );
			resized();
			repaint();
		}

		PluginMode getMode() const {
			return mMode;
		}

		void setSilicaPeriod( float period ) {
			mSilicaPeriod = period;
			repaint();
		}

		void setScaleRoot( int root ) {
			mScaleRoot = root;
			repaint();
		}

		void setScaleType( int typeIndex ) {
			mScaleType = typeIndex;
			repaint();
		}

		int getPreferredHeight() const {
			int h = PAD_SM + ROW_H; // mode buttons row 1
			h += PAD_SM + ROW_H;    // mode buttons row 2
			if ( mMode == PluginMode::silica )
				h += PAD_SM + ROW_H; // period
			if ( mMode == PluginMode::scale )
				h += PAD_SM + ROW_H + PAD_SM + ROW_H; // root + scale type
			h += PAD_SM;
			return h;
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( colours::settingsBg );

			// Silica period row
			if ( mMode == PluginMode::silica ) {
				auto       r      = silicaPeriodRect();
				const bool active = mDragField == DragField::SilicaPeriod;
				if ( active ) {
					g.setColour( colours::inputBg );
					g.fillRoundedRectangle( r.toFloat(), BTN_CORNER_RADIUS );
				}
				g.setColour( colours::offWhite );
				g.setFont( FONT_SM );
				g.drawText( "Period", r.removeFromLeft( 48 ), ::juce::Justification::centredLeft );
				g.setColour( ::juce::Colours::white );
				g.setFont( FONT_LG );
				g.drawText( ::juce::String( mSilicaPeriod, 2 ) + " b", r, ::juce::Justification::centredLeft );
			}

			// Scale root + type rows
			if ( mMode == PluginMode::scale ) {
				// Root row
				{
					auto       r      = scaleRootRect();
					const bool active = mDragField == DragField::ScaleRoot;
					if ( active ) {
						g.setColour( colours::inputBg );
						g.fillRoundedRectangle( r.toFloat(), BTN_CORNER_RADIUS );
					}
					g.setColour( colours::offWhite );
					g.setFont( FONT_SM );
					g.drawText( "Root", r.removeFromLeft( 48 ), ::juce::Justification::centredLeft );
					g.setColour( ::juce::Colours::white );
					g.setFont( FONT_LG );
					g.drawText( music::NOTE_NAMES[ mScaleRoot ], r, ::juce::Justification::centredLeft );
				}
				// Scale type row
				{
					auto       r      = scaleTypeRect();
					const bool active = mDragField == DragField::ScaleType;
					if ( active ) {
						g.setColour( colours::inputBg );
						g.fillRoundedRectangle( r.toFloat(), BTN_CORNER_RADIUS );
					}
					g.setColour( colours::offWhite );
					g.setFont( FONT_SM );
					g.drawText( "Scale", r.removeFromLeft( 48 ), ::juce::Justification::centredLeft );
					g.setColour( ::juce::Colours::white );
					g.setFont( FONT_LG );
					g.drawText( music::SCALES[ static_cast<size_t>( mScaleType ) ].name, r, ::juce::Justification::centredLeft );
				}
			}

			// Bottom border
			g.setColour( colours::borderLine );
			g.drawHorizontalLine( getHeight() - 1, 0.0f, static_cast<float>( getWidth() ) );
		}

		void resized() override {
			auto bounds       = getLocalBounds();
			auto numberOfBtns = 5;
			bounds.removeFromTop( PAD_SM );
			auto buttonBounds = bounds.removeFromTop( ROW_H );

			const int btnW = ( buttonBounds.getWidth() - 2 * PAD_SM - ( numberOfBtns - 1 ) * PAD_SM ) / numberOfBtns;

			buttonBounds.removeFromLeft( PAD_SM );
			// Row 1: PluginMode buttons
			buttonBounds.removeFromLeft( PAD_SM / 2 );
			mBtnPro.setBounds( buttonBounds.removeFromLeft( btnW ) );
			buttonBounds.removeFromLeft( PAD_SM );
			mBtnMelody.setBounds( buttonBounds.removeFromLeft( btnW ) );
			buttonBounds.removeFromLeft( PAD_SM );
			mBtnDrums.setBounds( buttonBounds.removeFromLeft( btnW ) );
			buttonBounds.removeFromLeft( PAD_SM );
			mBtnSilica.setBounds( buttonBounds.removeFromLeft( btnW ) );
			buttonBounds.removeFromLeft( PAD_SM );
			mBtnScale.setBounds( buttonBounds.removeFromLeft( btnW ) );
		}

		void mouseDown( const ::juce::MouseEvent &e ) override {
			if ( mMode == PluginMode::silica && silicaPeriodRect().contains( e.getPosition() ) ) {
				mDragField      = DragField::SilicaPeriod;
				mDragStartY     = e.getPosition().y;
				mDragStartValue = mSilicaPeriod;
				setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
			} else if ( mMode == PluginMode::scale && scaleRootRect().contains( e.getPosition() ) ) {
				mDragField    = DragField::ScaleRoot;
				mDragStartY   = e.getPosition().y;
				mDragStartInt = mScaleRoot;
				setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
			} else if ( mMode == PluginMode::scale && scaleTypeRect().contains( e.getPosition() ) ) {
				mDragField    = DragField::ScaleType;
				mDragStartY   = e.getPosition().y;
				mDragStartInt = mScaleType;
				setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
			}
		}

		void mouseDrag( const ::juce::MouseEvent &e ) override {
			const int dy = mDragStartY - e.getPosition().y;
			if ( mDragField == DragField::SilicaPeriod ) {
				const float newVal = ::juce::jmax( 0.01f, mDragStartValue + dy * 0.05f );
				mSilicaPeriod      = newVal;
				if ( mCbs.onSilicaPeriodChanged )
					mCbs.onSilicaPeriodChanged( newVal );
				repaint();
			} else if ( mDragField == DragField::ScaleRoot ) {
				const int newRoot = ( ( mDragStartInt + dy / 6 ) % 12 + 12 ) % 12;
				if ( newRoot != mScaleRoot ) {
					mScaleRoot = newRoot;
					fireScaleChanged();
					repaint();
				}
			} else if ( mDragField == DragField::ScaleType ) {
				const int count   = static_cast<int>( music::SCALES.size() );
				const int newType = ::juce::jlimit( 0, count - 1, mDragStartInt + dy / 8 );
				if ( newType != mScaleType ) {
					mScaleType = newType;
					fireScaleChanged();
					repaint();
				}
			}
		}

		void mouseUp( const ::juce::MouseEvent & ) override {
			if ( mDragField != DragField::None )
				setMouseCursor( ::juce::MouseCursor::NormalCursor );
			mDragField = DragField::None;
		}

		void mouseDoubleClick( const ::juce::MouseEvent &e ) override {
			if ( mMode == PluginMode::silica && silicaPeriodRect().contains( e.getPosition() ) ) {
				startPeriodEdit();
			}
		}

	 private:
		static constexpr int ROW_H = 26;

		const Callbacks mCbs;

		::juce::TextButton mBtnPro{ "Pro" };
		::juce::TextButton mBtnMelody{ "Melody" };
		::juce::TextButton mBtnDrums{ "Drums" };
		::juce::TextButton mBtnSilica{ "Silica" };
		::juce::TextButton mBtnScale{ "Scale" };

		PluginMode mMode         = PluginMode::melody;
		float      mSilicaPeriod = 4.0f;
		int        mScaleRoot    = 0; // 0 = C
		int        mScaleType    = 1; // Major

		enum class DragField { None, SilicaPeriod, ScaleRoot, ScaleType };
		DragField mDragField      = DragField::None;
		int       mDragStartY     = 0;
		float     mDragStartValue = 0.0f;
		int       mDragStartInt   = 0;

		::juce::TextEditor mEditor;
		bool               mEditorActive = false;

		int modeSettingsY() const {
			return PAD_SM + ROW_H + PAD_SM + ROW_H + PAD_SM;
		}

		::juce::Rectangle<int> silicaPeriodRect() const {
			return { PAD_SM, modeSettingsY(), getWidth() - 2 * PAD_SM, ROW_H };
		}

		::juce::Rectangle<int> scaleRootRect() const {
			return { PAD_SM, modeSettingsY(), getWidth() - 2 * PAD_SM, ROW_H };
		}

		::juce::Rectangle<int> scaleTypeRect() const {
			return { PAD_SM, modeSettingsY() + ROW_H + PAD_SM, getWidth() - 2 * PAD_SM, ROW_H };
		}

		void fireMode( PluginMode mode ) {
			if ( mCbs.onModeChanged )
				mCbs.onModeChanged( mode );
		}

		void fireScaleChanged() {
			if ( mCbs.onScaleChanged )
				mCbs.onScaleChanged( mScaleRoot, mScaleType );
		}

		void startPeriodEdit() {
			if ( mEditorActive )
				return;
			mEditorActive     = true;
			const auto bounds = silicaPeriodRect().withTrimmedLeft( 48 ).reduced( 2 );
			mEditor.setJustification( ::juce::Justification::centred );
			mEditor.setColour( ::juce::TextEditor::backgroundColourId, colours::inputBg );
			mEditor.setColour( ::juce::TextEditor::textColourId, ::juce::Colours::white );
			mEditor.setColour( ::juce::TextEditor::outlineColourId, colours::accentBlue );
			mEditor.setInputRestrictions( 8, "0123456789." );
			mEditor.setBounds( bounds );
			mEditor.setText( ::juce::String( mSilicaPeriod ), false );
			mEditor.onReturnKey = [ this ] { commitPeriodEdit(); };
			mEditor.onEscapeKey = [ this ] { cancelPeriodEdit(); };
			mEditor.onFocusLost = [ this ] { commitPeriodEdit(); };
			addAndMakeVisible( mEditor );
			mEditor.grabKeyboardFocus();
			mEditor.selectAll();
		}

		void commitPeriodEdit() {
			if ( !mEditorActive )
				return;
			const float newVal = ::juce::jmax( 0.01f, mEditor.getText().getFloatValue() );
			mSilicaPeriod      = newVal;
			if ( mCbs.onSilicaPeriodChanged )
				mCbs.onSilicaPeriodChanged( newVal );
			cancelPeriodEdit();
		}

		void cancelPeriodEdit() {
			mEditorActive = false;
			removeChildComponent( &mEditor );
			repaint();
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( SettingsPanel )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_SETTINGS_PANEL_HPP
