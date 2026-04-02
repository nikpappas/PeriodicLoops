#ifndef PLOP_SRC_UI_GROUP_STRIP_HPP
#define PLOP_SRC_UI_GROUP_STRIP_HPP

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/midi.hpp"
#include "ui/colours.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	/// Horizontal strip of colour swatches — one per group, plus an Add button.
	/// Clicking a swatch toggles that group's expanded state.
	class GroupStrip : public ::juce::Component {
	 public:
		GroupStrip() = default;

		std::function<void( int )> onGroupClicked; ///< toggle expand/collapse
		std::function<void()>      onAddGroup;
		std::function<void( int )> onRemoveGroup; ///< right-click to remove

		void setGroups( const ::std::vector<NoteGroup> &groups ) {
			mGroups = groups;
		}

		void paint( ::juce::Graphics &g ) override {
			const int swatchSize = SWATCH_SIZE;
			int       x          = PAD_MD;

			for ( int i = 0; i < static_cast<int>( mGroups.size() ); ++i ) {
				const auto &group  = mGroups[ i ];
				const auto  rect   = ::juce::Rectangle<int>( x, ( getHeight() - swatchSize ) / 2, swatchSize, swatchSize );
				const float corner = swatchSize / 2.0f;

				if ( group.expanded ) {
					// Selected indicator: brighter border
					g.setColour( group.colour.brighter( 0.5f ) );
					g.drawRoundedRectangle( rect.toFloat().expanded( 2.0f ), corner + 1.0f, 2.0f );
				}

				g.setColour( group.muted ? group.colour.withAlpha( 0.3f ) : group.colour );
				g.fillRoundedRectangle( rect.toFloat(), corner );

				// Group number
				g.setColour( ::juce::Colours::white.withAlpha( 0.8f ) );
				g.setFont( FONT_SM );
				g.drawText( ::juce::String( i + 1 ), rect, ::juce::Justification::centred );

				x += swatchSize + PAD_SM;
			}

			// Add button
			if ( static_cast<int>( mGroups.size() ) < MAX_GROUPS ) {
				const auto addRect = ::juce::Rectangle<int>( x, ( getHeight() - swatchSize ) / 2, swatchSize, swatchSize );
				g.setColour( colours::addAccent );
				g.setFont( FONT_LG );
				g.drawText( "+", addRect, ::juce::Justification::centred );
			}

			// Bottom border
			g.setColour( colours::borderLine );
			g.drawHorizontalLine( getHeight() - 1, 0.0f, static_cast<float>( getWidth() ) );
		}

		void mouseDown( const ::juce::MouseEvent &e ) override {
			const int swatchSize = SWATCH_SIZE;
			int       x          = PAD_MD;

			for ( int i = 0; i < static_cast<int>( mGroups.size() ); ++i ) {
				const auto rect = ::juce::Rectangle<int>( x, ( getHeight() - swatchSize ) / 2, swatchSize, swatchSize );
				if ( rect.contains( e.getPosition() ) ) {
					if ( e.mods.isPopupMenu() && onRemoveGroup )
						onRemoveGroup( i );
					else if ( onGroupClicked )
						onGroupClicked( i );
					return;
				}
				x += swatchSize + PAD_SM;
			}

			// Add button
			if ( static_cast<int>( mGroups.size() ) < MAX_GROUPS ) {
				const auto addRect = ::juce::Rectangle<int>( x, ( getHeight() - swatchSize ) / 2, swatchSize, swatchSize );
				if ( addRect.contains( e.getPosition() ) && onAddGroup )
					onAddGroup();
			}
		}

		static constexpr int STRIP_HEIGHT = 30;

	 private:
		static constexpr int SWATCH_SIZE = 22;
		static constexpr int MAX_GROUPS  = 8;

		::std::vector<NoteGroup> mGroups;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( GroupStrip )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_GROUP_STRIP_HPP
