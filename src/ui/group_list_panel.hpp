#ifndef PLOP_SRC_UI_GROUP_LIST_PANEL_HPP
#define PLOP_SRC_UI_GROUP_LIST_PANEL_HPP

#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/drums.hpp"
#include "music/midi.hpp"
#include "music/patterns.hpp"
#include "processor/plugin_state.hpp"
#include "ui/colours.hpp"
#include "ui/group_panel.hpp"
#include "ui/group_strip.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

   /// Container: GroupStrip (top) + scrollable list of GroupPanels.
   /// Replaces NoteListPanel in Drums/Silica/Melody modes.
   class GroupListPanel : public ::juce::Component {
    public:
      struct Callbacks {
         std::function<void( const ::std::vector<::plop::NoteGroup> & )> onGroupsChanged;
      };

      explicit GroupListPanel( Callbacks cbs ) : mCbs( std::move( cbs ) ) {
         mStrip.onGroupClicked = [ this ]( int i ) { toggleExpand( i ); };
         mStrip.onAddGroup     = [ this ] { addGroup(); };
         mStrip.onRemoveGroup  = [ this ]( int i ) { removeGroup( i ); };

         addAndMakeVisible( mStrip );
         mViewport.setScrollBarsShown( true, false );
         mViewport.setViewedComponent( &mPanelContainer, false );
         addAndMakeVisible( mViewport );
      }

      /// Full replacement — rebuilds all panel widgets. Use for mode switch / state load.
      void setGroups( const ::std::vector<::plop::NoteGroup> &groups ) {
         mGroups = groups;
         rebuildPanels();
      }

      void setScaleConstraint( int root, int scaleType ) {
         mScaleRoot = root;
         mScaleType = scaleType;
      }

      const ::std::vector<::plop::NoteGroup> &getGroups() const {
         return mGroups;
      }

      void expandGroup( int index ) {
         toggleExpand( index );
      }

      void paint( ::juce::Graphics &g ) override {
         g.fillAll( colours::panelBg );

         // Header
         g.setColour( colours::noteHeaderBg );
         g.fillRect( 0, 0, getWidth(), HEADER_H );
         g.setColour( ::juce::Colours::white );
         g.setFont( ::juce::Font( FONT_LG, ::juce::Font::bold ) );
         g.drawText( "Groups", PAD_MD, 0, getWidth() - PAD_MD, HEADER_H, ::juce::Justification::centredLeft );
      }

      void resized() override {
         mStrip.setBounds( 0, HEADER_H, getWidth(), GroupStrip::STRIP_HEIGHT );
         const int contentY = HEADER_H + GroupStrip::STRIP_HEIGHT;
         mViewport.setBounds( 0, contentY, getWidth(), getHeight() - contentY );
         layoutPanels();
      }

    private:
      static constexpr int HEADER_H = 30;

      const Callbacks                              mCbs;
      GroupStrip                                   mStrip;
      ::juce::Viewport                             mViewport;
      ::juce::Component                            mPanelContainer;
      ::std::vector<::std::unique_ptr<GroupPanel>> mPanels;
      ::std::vector<NoteGroup>                     mGroups;
      int                                          mScaleRoot = 0;
      int                                          mScaleType = 1;

      /// Notify parent of data change (does NOT rebuild panels).
      void fireChanged() {
         if ( mCbs.onGroupsChanged )
            mCbs.onGroupsChanged( mGroups );
      }

      void toggleExpand( int index ) {
         if ( index < 0 || index >= static_cast<int>( mGroups.size() ) )
            return;
         mGroups[ index ].expanded = !mGroups[ index ].expanded;
         fireChanged();
         rebuildPanels();
      }

      void addGroup() {
         if ( static_cast<int>( mGroups.size() ) >= 8 )
            return;

         static const ::juce::Colour palette[] = {
            colours::paletteBlue,   colours::paletteRed,  colours::paletteGreen, colours::paletteOrange,
            colours::palettePurple, colours::paletteTeal, colours::palettePink,  colours::paletteBrown,
         };

         NoteGroup newGroup;
         newGroup.colour    = palette[ mGroups.size() % std::size( palette ) ];
         newGroup.period    = 4.0f;
         newGroup.rootPitch = 60;
         newGroup.noteCount = 1;
         newGroup.mode      = ::plop::PluginMode::Melody;

         mGroups.push_back( newGroup );
         fireChanged();
         rebuildPanels();
      }

      void removeGroup( int index ) {
         if ( index < 0 || index >= static_cast<int>( mGroups.size() ) )
            return;
         if ( mGroups.size() <= 1 )
            return;
         mGroups.erase( mGroups.begin() + index );
         fireChanged();
         rebuildPanels();
      }

      void rebuildPanels() {
         mStrip.setGroups( mGroups );
         mStrip.repaint();

         for ( auto &p : mPanels )
            mPanelContainer.removeChildComponent( p.get() );
         mPanels.clear();

         for ( int i = 0; i < static_cast<int>( mGroups.size() ); ++i ) {
            auto panel = std::make_unique<GroupPanel>();
            panel->setGroup( mGroups[ i ], i );
            panel->setScaleConstraint( mScaleRoot, mScaleType );
            wirePanel( i, *panel );
            mPanelContainer.addAndMakeVisible( *panel );
            mPanels.push_back( std::move( panel ) );
         }

         layoutPanels();
      }

      /// Wire callbacks for a single panel. Structural changes rebuild; value changes don't.
      void wirePanel( int idx, GroupPanel &panel ) {
         // Value change: read back panel state, re-layout (handles height changes on mode switch), notify parent.
         panel.onGroupChanged = [ this, idx ] {
            if ( idx < static_cast<int>( mPanels.size() ) ) {
               mGroups[ idx ] = mPanels[ idx ]->getGroup();
               mStrip.setGroups( mGroups );
               mStrip.repaint();
               layoutPanels();
               fireChanged();
            }
         };

         panel.onToggleMute = [ this, idx ] {
            if ( idx < static_cast<int>( mGroups.size() ) ) {
               mGroups[ idx ].muted = !mGroups[ idx ].muted;
               fireChanged();
               rebuildPanels();
            }
         };

         panel.onToggleSolo = [ this, idx ]( bool ctrl ) {
            if ( idx < static_cast<int>( mGroups.size() ) ) {
               if ( ctrl ) {
                  mGroups[ idx ].solo = !mGroups[ idx ].solo;
               } else {
                  const bool wasAlreadySolo = mGroups[ idx ].solo;
                  for ( auto &g : mGroups )
                     g.solo = false;
                  if ( !wasAlreadySolo )
                     mGroups[ idx ].solo = true;
               }
               fireChanged();
               rebuildPanels();
            }
         };

         panel.onAddVoice = [ this, idx ] {
            if ( idx < static_cast<int>( mGroups.size() ) && mGroups[ idx ].mode == ::plop::PluginMode::Drums ) {
               auto &g = mGroups[ idx ];
               g.voices.push_back( PeriodicNote{
                 .pitch    = g.rootPitch,
                 .period   = g.period,
                 .offset   = 0.0f,
                 .duration = 0.25f,
                 .channel  = g.channel,
               } );
               fireChanged();
               rebuildPanels();
            }
         };
         panel.onRemoveVoice = [ this, idx ]( int vi ) {
            if ( idx < static_cast<int>( mGroups.size() ) && mGroups[ idx ].mode == ::plop::PluginMode::Drums
                 && vi < static_cast<int>( mGroups[ idx ].voices.size() ) ) {
               mGroups[ idx ].voices.erase( mGroups[ idx ].voices.begin() + vi );
               fireChanged();
               rebuildPanels();
            }
         };
      }

      void layoutPanels() {
         int y = 0;
         for ( int i = 0; i < static_cast<int>( mPanels.size() ); ++i ) {
            mPanels[ i ]->setGroup( mGroups[ i ], i );
            const int panelH = mPanels[ i ]->getPreferredHeight();
            mPanels[ i ]->setBounds( 0, y, mViewport.getWidth(), panelH );
            y += panelH;
         }
         mPanelContainer.setSize( mViewport.getWidth(), y );
      }

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( GroupListPanel )
   };

} // namespace plop::ui

#endif // PLOP_SRC_UI_GROUP_LIST_PANEL_HPP
