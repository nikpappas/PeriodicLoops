#ifndef PLOP_SRC_UI_SETTINGS_PANEL_HPP
#define PLOP_SRC_UI_SETTINGS_PANEL_HPP

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/scales.hpp"
#include "processor/plugin_state.hpp"
#include "ui/colours.hpp"
#include "ui/note_list_panel.hpp"

namespace plop::ui {

	class SettingsPanel : public ::juce::Component {
	 public:
		struct Callbacks {
			std::function<void( PluginMode )> onModeChanged;
			std::function<void( float )>      onSilicaPeriodChanged;
			std::function<void( int, int )>   onScaleChanged; // root, typeIndex
			std::function<void()>             onPlayPauseToggled;
		};

		explicit SettingsPanel( Callbacks cbs, bool showPlayPause ) :
				  mCbs( std::move( cbs ) ), mShowPlayPause( showPlayPause ) {
			for ( auto *btn : { &mBtnPro, &mBtnMelody, &mBtnDrums, &mBtnSilica, &mBtnScale } ) {
				btn->setClickingTogglesState( false );
				btn->setColour( ::juce::TextButton::buttonOnColourId, colours::btnAccentColour );
				addAndMakeVisible( *btn );
			}

			mBtnPro.onClick    = [ this ] { fireMode( PluginMode::Pro ); };
			mBtnMelody.onClick = [ this ] { fireMode( PluginMode::Melody ); };
			mBtnDrums.onClick  = [ this ] { fireMode( PluginMode::Drums ); };
			mBtnSilica.onClick = [ this ] { fireMode( PluginMode::Silica ); };
			mBtnScale.onClick  = [ this ] { fireMode( PluginMode::Scale ); };

			if ( mShowPlayPause ) {
				mBtnPlayPause.setColour( ::juce::TextButton::buttonOnColourId, colours::btnAccentColourAlt );
				mBtnPlayPause.setClickingTogglesState( false );
				mBtnPlayPause.onClick = [ this ] {
					if ( mCbs.onPlayPauseToggled )
						mCbs.onPlayPauseToggled();
				};
				addAndMakeVisible( mBtnPlayPause );
			}
		}

		void setMode( PluginMode mode ) {
			mMode = mode;
			mBtnPro.setToggleState( mode == PluginMode::Pro, ::juce::dontSendNotification );
			mBtnMelody.setToggleState( mode == PluginMode::Melody, ::juce::dontSendNotification );
			mBtnDrums.setToggleState( mode == PluginMode::Drums, ::juce::dontSendNotification );
			mBtnSilica.setToggleState( mode == PluginMode::Silica, ::juce::dontSendNotification );
			mBtnScale.setToggleState( mode == PluginMode::Scale, ::juce::dontSendNotification );
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

		void setPlaying( bool playing ) {
			mPlaying = playing;
			mBtnPlayPause.setButtonText( playing ? "Pause" : "Play" );
			mBtnPlayPause.setToggleState( playing, ::juce::dontSendNotification );
		}

		int getPreferredHeight() const {
			int h = kPadding + kRowH; // mode buttons row 1
			h += kPadding + kRowH;    // mode buttons row 2
			if ( mMode == PluginMode::Silica )
				h += kPadding + kRowH; // period
			if ( mMode == PluginMode::Scale )
				h += kPadding + kRowH + kPadding + kRowH; // root + scale type
			if ( mShowPlayPause )
				h += kPadding + kRowH; // play/pause
			h += kPadding;
			return h;
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( ::juce::Colour( 0xff181828 ) );

			// Silica period row
			if ( mMode == PluginMode::Silica ) {
				auto       r      = silicaPeriodRect();
				const bool active = mDragField == DragField::SilicaPeriod;
				if ( active ) {
					g.setColour( ::juce::Colour( 0xff2a2a44 ) );
					g.fillRoundedRectangle( r.toFloat(), 3.0f );
				}
				g.setColour( ::juce::Colour( 0xff888899 ) );
				g.setFont( 11.0f );
				g.drawText( "Period", r.removeFromLeft( 48 ), ::juce::Justification::centredLeft );
				g.setColour( ::juce::Colours::white );
				g.setFont( 13.0f );
				g.drawText( ::juce::String( mSilicaPeriod, 2 ) + " b", r, ::juce::Justification::centredLeft );
			}

			// Scale root + type rows
			if ( mMode == PluginMode::Scale ) {
				// Root row
				{
					auto       r      = scaleRootRect();
					const bool active = mDragField == DragField::ScaleRoot;
					if ( active ) {
						g.setColour( ::juce::Colour( 0xff2a2a44 ) );
						g.fillRoundedRectangle( r.toFloat(), 3.0f );
					}
					g.setColour( ::juce::Colour( 0xff888899 ) );
					g.setFont( 11.0f );
					g.drawText( "Root", r.removeFromLeft( 48 ), ::juce::Justification::centredLeft );
					g.setColour( ::juce::Colours::white );
					g.setFont( 13.0f );
					g.drawText( music::k_noteNames[ mScaleRoot ], r, ::juce::Justification::centredLeft );
				}
				// Scale type row
				{
					auto       r      = scaleTypeRect();
					const bool active = mDragField == DragField::ScaleType;
					if ( active ) {
						g.setColour( ::juce::Colour( 0xff2a2a44 ) );
						g.fillRoundedRectangle( r.toFloat(), 3.0f );
					}
					g.setColour( ::juce::Colour( 0xff888899 ) );
					g.setFont( 11.0f );
					g.drawText( "Scale", r.removeFromLeft( 48 ), ::juce::Justification::centredLeft );
					g.setColour( ::juce::Colours::white );
					g.setFont( 13.0f );
					g.drawText( music::k_scales[ static_cast<size_t>( mScaleType ) ].name, r, ::juce::Justification::centredLeft );
				}
			}

			// Bottom border
			g.setColour( ::juce::Colour( 0xff333344 ) );
			g.drawHorizontalLine( getHeight() - 1, 0.0f, static_cast<float>( getWidth() ) );
		}

		void resized() override {
			auto bounds       = getLocalBounds();
			auto numberOfBtns = 5;
			bounds.removeFromTop( kPadding );
			auto buttonBounds = bounds.removeFromTop( kRowH );

			const int btnW = ( buttonBounds.getWidth() - 2 * kPadding - ( numberOfBtns - 1 ) * kBtnGap ) / numberOfBtns;

			buttonBounds.removeFromLeft( kPadding );
			// Row 1: PluginMode buttons
			buttonBounds.removeFromLeft( kBtnGap / 2 );
			mBtnPro.setBounds( buttonBounds.removeFromLeft( btnW ) );
			buttonBounds.removeFromLeft( kBtnGap );
			mBtnMelody.setBounds( buttonBounds.removeFromLeft( btnW ) );
			buttonBounds.removeFromLeft( kBtnGap );
			mBtnDrums.setBounds( buttonBounds.removeFromLeft( btnW ) );
			buttonBounds.removeFromLeft( kBtnGap );
			mBtnSilica.setBounds( buttonBounds.removeFromLeft( btnW ) );
			buttonBounds.removeFromLeft( kBtnGap );
			mBtnScale.setBounds( buttonBounds.removeFromLeft( btnW ) );

			if ( mShowPlayPause ) {
				const int ppY = getHeight() - kPadding - kRowH;
				mBtnPlayPause.setBounds( kPadding, ppY, getWidth() - 2 * kPadding, kRowH );
			}
		}

		void mouseDown( const ::juce::MouseEvent &e ) override {
			if ( mMode == PluginMode::Silica && silicaPeriodRect().contains( e.getPosition() ) ) {
				mDragField      = DragField::SilicaPeriod;
				mDragStartY     = e.getPosition().y;
				mDragStartValue = mSilicaPeriod;
				setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
			} else if ( mMode == PluginMode::Scale && scaleRootRect().contains( e.getPosition() ) ) {
				mDragField    = DragField::ScaleRoot;
				mDragStartY   = e.getPosition().y;
				mDragStartInt = mScaleRoot;
				setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
			} else if ( mMode == PluginMode::Scale && scaleTypeRect().contains( e.getPosition() ) ) {
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
				const int count   = static_cast<int>( music::k_scales.size() );
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
			if ( mMode == PluginMode::Silica && silicaPeriodRect().contains( e.getPosition() ) ) {
				startPeriodEdit();
			}
		}

	 private:
		static constexpr int kPadding = 6;
		static constexpr int kRowH    = 26;
		static constexpr int kBtnGap  = 6;

		const Callbacks mCbs;
		const bool      mShowPlayPause;

		::juce::TextButton mBtnPro{ "Pro" };
		::juce::TextButton mBtnMelody{ "Melody" };
		::juce::TextButton mBtnDrums{ "Drums" };
		::juce::TextButton mBtnSilica{ "Silica" };
		::juce::TextButton mBtnScale{ "Scale" };
		::juce::TextButton mBtnPlayPause{ "Play" };

		PluginMode mMode         = PluginMode::Melody;
		float      mSilicaPeriod = 4.0f;
		int        mScaleRoot    = 0; // 0 = C
		int        mScaleType    = 1; // Major
		bool       mPlaying      = true;

		enum class DragField { None, SilicaPeriod, ScaleRoot, ScaleType };
		DragField mDragField      = DragField::None;
		int       mDragStartY     = 0;
		float     mDragStartValue = 0.0f;
		int       mDragStartInt   = 0;

		::juce::TextEditor mEditor;
		bool               mEditorActive = false;

		int modeSettingsY() const {
			return kPadding + kRowH + kPadding + kRowH + kPadding;
		}

		::juce::Rectangle<int> silicaPeriodRect() const {
			return { kPadding, modeSettingsY(), getWidth() - 2 * kPadding, kRowH };
		}

		::juce::Rectangle<int> scaleRootRect() const {
			return { kPadding, modeSettingsY(), getWidth() - 2 * kPadding, kRowH };
		}

		::juce::Rectangle<int> scaleTypeRect() const {
			return { kPadding, modeSettingsY() + kRowH + kPadding, getWidth() - 2 * kPadding, kRowH };
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
			mEditor.setColour( ::juce::TextEditor::backgroundColourId, ::juce::Colour( 0xff2a2a44 ) );
			mEditor.setColour( ::juce::TextEditor::textColourId, ::juce::Colours::white );
			mEditor.setColour( ::juce::TextEditor::outlineColourId, ::juce::Colour( 0xff4fc3f7 ) );
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
