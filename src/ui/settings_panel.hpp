#ifndef PLOP_SRC_UI_SETTINGS_PANEL_HPP
#define PLOP_SRC_UI_SETTINGS_PANEL_HPP

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/scales.hpp"
#include "processor/plugin_state.hpp"
#include "ui/colours.hpp"
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
         for ( auto *btn : { &mBtnPro, &mBtnGroups } ) {
            btn->setClickingTogglesState( false );
            btn->setColour( ::juce::TextButton::buttonOnColourId, colours::btnAccentColour );
            addAndMakeVisible( *btn );
         }

         mBtnPro.onClick    = [ this ] { fireMode( PluginMode::Pro ); };
         mBtnGroups.onClick = [ this ] { fireMode( PluginMode::Melody ); };
      }

      void setMode( PluginMode mode ) {
         mMode = mode;
         mBtnPro.setToggleState( mode == PluginMode::Pro, ::juce::dontSendNotification );
         mBtnGroups.setToggleState( mode != PluginMode::Pro, ::juce::dontSendNotification );
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
         int h = PAD_SM + ROW_H; // mode buttons row
         if ( mMode != PluginMode::Pro )
            h += PAD_SM + ROW_H + PAD_SM + ROW_H; // scale root + scale type
         h += PAD_SM;
         return h;
      }

      void paint( ::juce::Graphics &g ) override {
         g.fillAll( colours::settingsBg );

         // Scale root + type rows (Groups mode only)
         if ( mMode != PluginMode::Pro ) {
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
         auto numberOfBtns = 2;
         bounds.removeFromTop( PAD_SM );
         auto buttonBounds = bounds.removeFromTop( ROW_H );

         const int btnW = ( buttonBounds.getWidth() - 2 * PAD_SM - ( numberOfBtns - 1 ) * PAD_SM ) / numberOfBtns;

         buttonBounds.removeFromLeft( PAD_SM );
         buttonBounds.removeFromLeft( PAD_SM / 2 );
         mBtnPro.setBounds( buttonBounds.removeFromLeft( btnW ) );
         buttonBounds.removeFromLeft( PAD_SM );
         mBtnGroups.setBounds( buttonBounds.removeFromLeft( btnW ) );
      }

      void mouseDown( const ::juce::MouseEvent &e ) override {
         if ( mMode != PluginMode::Pro && scaleRootRect().contains( e.getPosition() ) ) {
            mDragField    = DragField::ScaleRoot;
            mDragStartY   = e.getPosition().y;
            mDragStartInt = mScaleRoot;
            setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
         } else if ( mMode != PluginMode::Pro && scaleTypeRect().contains( e.getPosition() ) ) {
            mDragField    = DragField::ScaleType;
            mDragStartY   = e.getPosition().y;
            mDragStartInt = mScaleType;
            setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
         }
      }

      void mouseDrag( const ::juce::MouseEvent &e ) override {
         const int dy = mDragStartY - e.getPosition().y;
         if ( mDragField == DragField::ScaleRoot ) {
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

    private:
      static constexpr int ROW_H = 26;

      const Callbacks mCbs;

      ::juce::TextButton mBtnPro{ "Pro" };
      ::juce::TextButton mBtnGroups{ "Groups" };

      PluginMode mMode         = PluginMode::Melody;
      float      mSilicaPeriod = 4.0f;
      int        mScaleRoot    = 0;
      int        mScaleType    = 1;

      enum class DragField { None, ScaleRoot, ScaleType };
      DragField mDragField    = DragField::None;
      int       mDragStartY   = 0;
      int       mDragStartInt = 0;

      int modeSettingsY() const {
         return PAD_SM + ROW_H + PAD_SM;
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

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( SettingsPanel )
   };

} // namespace plop::ui

#endif // PLOP_SRC_UI_SETTINGS_PANEL_HPP
