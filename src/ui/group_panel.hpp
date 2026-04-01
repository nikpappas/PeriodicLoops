#ifndef PLOP_SRC_UI_GROUP_PANEL_HPP
#define PLOP_SRC_UI_GROUP_PANEL_HPP

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/drums.hpp"
#include "music/midi.hpp"
#include "music/patterns.hpp"
#include "music/scales.hpp"
#include "processor/plugin_state.hpp"
#include "ui/colours.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	inline ::juce::String groupPanelPitchName( int pitch ) {
		static constexpr const char *names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
		return ::juce::String( names[ pitch % 12 ] ) + ::juce::String( pitch / 12 - 2 );
	}

	/// Expandable panel for a single NoteGroup. Renders header + mode-specific body.
	class GroupPanel : public ::juce::Component {
	 public:
		GroupPanel() = default;

		std::function<void()>           onGroupChanged;
		std::function<void( int, int )> onVoiceChanged; ///< (groupIndex, voiceIndex)
		std::function<void()>           onAddVoice;
		std::function<void( int )>      onRemoveVoice;
		std::function<void()>           onToggleMute;
		std::function<void( bool )>     onToggleSolo; ///< bool = ctrl held

		void setGroup( const NoteGroup &group, int index ) {
			mGroup = group;
			mIndex = index;
		}

		void setScaleConstraint( int root, int scaleType ) {
			mScaleRoot = root;
			mScaleType = scaleType;
		}

		const NoteGroup &getGroup() const {
			return mGroup;
		}

		int getPreferredHeight() const {
			if ( !mGroup.expanded )
				return HEADER_H;

			int h = HEADER_H;
			h += CONTROLS_ROW_H; // pattern/period/channel controls row

			if ( mGroup.mode == ::plop::PluginMode::Drums ) {
				h += static_cast<int>( mGroup.voices.size() ) * VOICE_ROW_H;
				h += ADD_VOICE_H;
			} else {
				// Silica / Melody: preview row
				h += PREVIEW_H;
			}

			return h + PAD_SM;
		}

		void paint( ::juce::Graphics &g ) override {
			// Background
			g.fillAll( colours::panelBg );

			// Header
			const auto headerBounds = ::juce::Rectangle<int>( 0, 0, getWidth(), HEADER_H );
			g.setColour( mGroup.colour.withAlpha( 0.25f ) );
			g.fillRect( headerBounds );

			// Expand arrow
			g.setColour( ::juce::Colours::white );
			g.setFont( FONT_LG );
			g.drawText( mGroup.expanded ? "v" : ">", PAD_SM, 0, 14, HEADER_H, ::juce::Justification::centred );

			// Colour swatch
			const auto swatchRect = ::juce::Rectangle<int>( 18, ( HEADER_H - 14 ) / 2, 14, 14 );
			g.setColour( mGroup.colour );
			g.fillRoundedRectangle( swatchRect.toFloat(), 7.0f );

			// Group name
			g.setColour( ::juce::Colours::white );
			g.setFont( FONT_LG );
			g.drawText( "Group " + ::juce::String( mIndex + 1 ), 38, 0, 70, HEADER_H, ::juce::Justification::centredLeft );

			// Period display
			g.setColour( colours::darkestGrey );
			g.setFont( FONT_SM );
			g.drawText( ::juce::String( mGroup.period, 2 ) + "b", 110, 0, 45, HEADER_H, ::juce::Justification::centredLeft );

			// Channel display
			g.drawText( "ch" + ::juce::String( mGroup.channel + 1 ), 155, 0, 30, HEADER_H, ::juce::Justification::centredLeft );

			// Mode buttons D/S/M
			static constexpr const char        *modeLbls[] = { "D", "S", "M" };
			static constexpr ::plop::PluginMode modeVals[] = { ::plop::PluginMode::Drums,
				                                                ::plop::PluginMode::Silica,
				                                                ::plop::PluginMode::Melody };

			for ( int m = 0; m < 3; ++m ) {
				const auto r      = modeBtnRect( m );
				const bool active = mGroup.mode == modeVals[ m ];
				g.setColour( active ? mGroup.colour.withAlpha( 0.7f ) : colours::inputBg );
				g.fillRoundedRectangle( r.toFloat(), BTN_CORNER_RADIUS );
				g.setColour( active ? ::juce::Colours::white : colours::offWhite );
				g.setFont( FONT_SM );
				g.drawText( modeLbls[ m ], r, ::juce::Justification::centred );
			}

			// Mute/Solo buttons
			drawHeaderButton( g, muteRect(), "M", mGroup.muted, colours::removeAccent );
			drawHeaderButton( g, soloRect(), "S", mGroup.solo, colours::accentBlue );

			if ( !mGroup.expanded ) {
				g.setColour( colours::borderLine );
				g.drawHorizontalLine( getHeight() - 1, 0.0f, static_cast<float>( getWidth() ) );
				return;
			}

			// Controls row
			int y = HEADER_H;
			paintControlsRow( g, y );
			y += CONTROLS_ROW_H;

			if ( mGroup.mode == ::plop::PluginMode::Drums ) {
				paintDrumVoices( g, y );
			} else {
				paintPreview( g, y );
			}

			g.setColour( colours::borderLine );
			g.drawHorizontalLine( getHeight() - 1, 0.0f, static_cast<float>( getWidth() ) );
		}

		void mouseDown( const ::juce::MouseEvent &e ) override {
			const auto pos = e.getPosition();

			// Mute button
			if ( muteRect().contains( pos ) ) {
				if ( onToggleMute )
					onToggleMute();
				return;
			}

			// Solo button
			if ( soloRect().contains( pos ) ) {
				if ( onToggleSolo )
					onToggleSolo( e.mods.isCtrlDown() );
				return;
			}

			// Mode buttons D/S/M
			if ( pos.y < HEADER_H ) {
				static constexpr ::plop::PluginMode modeVals[] = { ::plop::PluginMode::Drums,
					                                                ::plop::PluginMode::Silica,
					                                                ::plop::PluginMode::Melody };
				for ( int m = 0; m < 3; ++m ) {
					if ( modeBtnRect( m ).contains( pos ) ) {
						mGroup.mode = modeVals[ m ];
						if ( onGroupChanged )
							onGroupChanged();
						repaint();
						return;
					}
				}
			}

			// Header: period drag region (x 110..155)
			if ( pos.y < HEADER_H && pos.x >= 110 && pos.x < 155 ) {
				startDrag( DragField::Period, pos.y, mGroup.period );
				return;
			}
			// Header: channel drag region (x 155..190)
			if ( pos.y < HEADER_H && pos.x >= 155 && pos.x < 190 ) {
				startDrag( DragField::Channel, pos.y, mGroup.channel );
				return;
			}
			// Header click -> expand/collapse (only on left portion)
			if ( pos.y < HEADER_H && pos.x < 110 ) {
				mGroup.expanded = !mGroup.expanded;
				if ( onGroupChanged )
					onGroupChanged();
				return;
			}

			if ( !mGroup.expanded )
				return;

			int y = HEADER_H;

			// Controls row: pattern function cycling
			if ( pos.y >= y && pos.y < y + CONTROLS_ROW_H ) {
				handleControlsClick( pos, y );
				return;
			}
			y += CONTROLS_ROW_H;

			// Drum voice rows
			if ( mGroup.mode == ::plop::PluginMode::Drums ) {
				for ( int i = 0; i < static_cast<int>( mGroup.voices.size() ); ++i ) {
					const auto removeR = ::juce::Rectangle<int>( getWidth() - PAD_MD - 16, y + ( VOICE_ROW_H - 16 ) / 2, 16, 16 );
					if ( removeR.contains( pos ) ) {
						if ( onRemoveVoice )
							onRemoveVoice( i );
						return;
					}
					// Voice pitch drag (x: PAD_MD .. PAD_MD+100)
					if ( pos.y >= y && pos.y < y + VOICE_ROW_H && pos.x >= PAD_MD && pos.x < PAD_MD + 100 ) {
						mDragVoiceIdx = i;
						startDrag( DragField::VoicePitch, pos.y, mGroup.voices[ i ].pitch );
						return;
					}
					// Voice offset drag (x: PAD_MD+105 .. PAD_MD+155)
					if ( pos.y >= y && pos.y < y + VOICE_ROW_H && pos.x >= PAD_MD + 105 && pos.x < PAD_MD + 155 ) {
						mDragVoiceIdx = i;
						startDrag( DragField::VoiceOffset, pos.y, mGroup.voices[ i ].offset );
						return;
					}
					// Voice duration drag (x: PAD_MD+160 .. PAD_MD+210)
					if ( pos.y >= y && pos.y < y + VOICE_ROW_H && pos.x >= PAD_MD + 160 && pos.x < PAD_MD + 210 ) {
						mDragVoiceIdx = i;
						startDrag( DragField::VoiceDuration, pos.y, mGroup.voices[ i ].duration );
						return;
					}
					y += VOICE_ROW_H;
				}
				// Add voice button
				const auto addRect = ::juce::Rectangle<int>( PAD_MD, y + 2, getWidth() - 2 * PAD_MD, ADD_VOICE_H - 4 );
				if ( addRect.contains( pos ) && onAddVoice )
					onAddVoice();
			}
		}

		void mouseDrag( const ::juce::MouseEvent &e ) override {
			if ( mDragField == DragField::None )
				return;
			const int dy = mDragStartY - e.getPosition().y;

			switch ( mDragField ) {
				case DragField::Period:
					mGroup.period = ::juce::jmax( 0.01f, mDragStartFloat + dy * 0.05f );
					break;
				case DragField::Channel:
					mGroup.channel = ::juce::jlimit( 0, 15, mDragStartInt + dy / 8 );
					break;
				case DragField::RootPitch: {
					if ( mGroup.mode == ::plop::PluginMode::Drums ) {
						const int startIdx = music::gmDrumIndexForNote( mDragStartInt );
						const int newIdx = ::juce::jlimit( 0, static_cast<int>( music::kGmDrums.size() ) - 1, startIdx + dy / 4 );
						mGroup.rootPitch = music::gmDrumNoteAtIndex( newIdx );
					} else {
						mGroup.rootPitch = ::juce::jlimit( 0, 127, mDragStartInt + dy / 3 );
					}
					break;
				}
				case DragField::NoteCount:
					mGroup.noteCount = ::juce::jlimit( 1, 16, mDragStartInt + dy / 6 );
					break;
				case DragField::VoicePitch: {
					if ( mDragVoiceIdx >= 0 && mDragVoiceIdx < static_cast<int>( mGroup.voices.size() ) ) {
						const int startIdx = music::gmDrumIndexForNote( mDragStartInt );
						const int newIdx = ::juce::jlimit( 0, static_cast<int>( music::kGmDrums.size() ) - 1, startIdx + dy / 4 );
						mGroup.voices[ mDragVoiceIdx ].pitch = music::gmDrumNoteAtIndex( newIdx );
					}
					break;
				}
				case DragField::VoiceOffset: {
					if ( mDragVoiceIdx >= 0 && mDragVoiceIdx < static_cast<int>( mGroup.voices.size() ) )
						mGroup.voices[ mDragVoiceIdx ].offset = ::juce::jlimit( 0.0f, mGroup.period, mDragStartFloat + dy * 0.05f );
					break;
				}
				case DragField::VoiceDuration: {
					if ( mDragVoiceIdx >= 0 && mDragVoiceIdx < static_cast<int>( mGroup.voices.size() ) )
						mGroup.voices[ mDragVoiceIdx ].duration = ::juce::jmax( 0.01f, mDragStartFloat + dy * 0.05f );
					break;
				}
				default:
					break;
			}

			if ( onGroupChanged )
				onGroupChanged();
			repaint();
		}

		void mouseUp( const ::juce::MouseEvent & ) override {
			if ( mDragField != DragField::None )
				setMouseCursor( ::juce::MouseCursor::NormalCursor );
			mDragField    = DragField::None;
			mDragVoiceIdx = -1;
		}

	 private:
		static constexpr int HEADER_H       = 28;
		static constexpr int CONTROLS_ROW_H = 26;
		static constexpr int VOICE_ROW_H    = 24;
		static constexpr int PREVIEW_H      = 40;
		static constexpr int ADD_VOICE_H    = 24;

		NoteGroup mGroup;
		int       mIndex     = 0;
		int       mScaleRoot = 0;
		int       mScaleType = 1;

		enum class DragField { None, Period, Channel, RootPitch, NoteCount, VoicePitch, VoiceOffset, VoiceDuration };
		DragField mDragField      = DragField::None;
		int       mDragStartY     = 0;
		float     mDragStartFloat = 0.0f;
		int       mDragStartInt   = 0;
		int       mDragVoiceIdx   = -1;

		::juce::Rectangle<int> muteRect() const {
			return { getWidth() - 52, ( HEADER_H - 18 ) / 2, 22, 18 };
		}
		::juce::Rectangle<int> soloRect() const {
			return { getWidth() - 26, ( HEADER_H - 18 ) / 2, 22, 18 };
		}
		/// m = 0 (Drums), 1 (Silica), 2 (Melody) — placed left of the Mute button.
		::juce::Rectangle<int> modeBtnRect( int m ) const {
			return { getWidth() - 114 + m * 20, ( HEADER_H - 16 ) / 2, 18, 16 };
		}

		void drawHeaderButton( ::juce::Graphics      &g,
		                       ::juce::Rectangle<int> r,
		                       const ::juce::String  &label,
		                       bool                   active,
		                       ::juce::Colour         col ) const {
			g.setColour( active ? col.withAlpha( 0.4f ) : colours::inputBg );
			g.fillRoundedRectangle( r.toFloat(), BTN_CORNER_RADIUS );
			g.setColour( active ? col : colours::offWhite );
			g.setFont( FONT_SM );
			g.drawText( label, r, ::juce::Justification::centred );
		}

		void paintControlsRow( ::juce::Graphics &g, int y ) const {
			g.setColour( colours::rowAlt );
			g.fillRect( 0, y, getWidth(), CONTROLS_ROW_H );

			const int lx = PAD_MD;
			g.setFont( FONT_SM );

			if ( mGroup.mode == ::plop::PluginMode::Drums ) {
				// Pattern | Notes | Period | Channel
				g.setColour( colours::offWhite );
				g.drawText( "Pat:", lx, y, 26, CONTROLS_ROW_H, ::juce::Justification::centredLeft );
				g.setColour( ::juce::Colours::white );
				g.drawText(
				  music::patternFunctionName( mGroup.pattern ), lx + 26, y, 70, CONTROLS_ROW_H, ::juce::Justification::centredLeft );

				g.setColour( colours::offWhite );
				g.drawText( "N:", lx + 100, y, 16, CONTROLS_ROW_H, ::juce::Justification::centredLeft );
				g.setColour( ::juce::Colours::white );
				g.drawText( ::juce::String( mGroup.noteCount ), lx + 116, y, 20, CONTROLS_ROW_H, ::juce::Justification::centredLeft );
			} else {
				// Root | Pattern | Notes
				g.setColour( colours::offWhite );
				g.drawText( "Root:", lx, y, 32, CONTROLS_ROW_H, ::juce::Justification::centredLeft );
				g.setColour( ::juce::Colours::white );
				g.drawText( groupPanelPitchName( mGroup.rootPitch ), lx + 32, y, 40, CONTROLS_ROW_H, ::juce::Justification::centredLeft );

				g.setColour( colours::offWhite );
				g.drawText( "Pat:", lx + 76, y, 26, CONTROLS_ROW_H, ::juce::Justification::centredLeft );
				g.setColour( ::juce::Colours::white );
				g.drawText(
				  music::patternFunctionName( mGroup.pattern ), lx + 102, y, 70, CONTROLS_ROW_H, ::juce::Justification::centredLeft );

				g.setColour( colours::offWhite );
				g.drawText( "N:", lx + 176, y, 16, CONTROLS_ROW_H, ::juce::Justification::centredLeft );
				g.setColour( ::juce::Colours::white );
				g.drawText( ::juce::String( mGroup.noteCount ), lx + 192, y, 20, CONTROLS_ROW_H, ::juce::Justification::centredLeft );
			}
		}

		void paintDrumVoices( ::juce::Graphics &g, int y ) const {
			g.setFont( FONT_SM );
			for ( int i = 0; i < static_cast<int>( mGroup.voices.size() ); ++i ) {
				const auto &voice = mGroup.voices[ i ];
				if ( i % 2 == 0 ) {
					g.setColour( colours::rowAlt );
					g.fillRect( 0, y, getWidth(), VOICE_ROW_H );
				}

				g.setColour( ::juce::Colours::white );
				const char *drumName = music::gmDrumName( voice.pitch );
				g.drawText( drumName ? ::juce::String( drumName ) : ::juce::String( voice.pitch ),
				            PAD_MD,
				            y,
				            100,
				            VOICE_ROW_H,
				            ::juce::Justification::centredLeft );

				g.setColour( colours::offWhite );
				g.drawText( ::juce::String( voice.offset, 2 ) + "b", PAD_MD + 105, y, 50, VOICE_ROW_H, ::juce::Justification::centredLeft );
				g.drawText( ::juce::String( voice.duration, 2 ) + "b", PAD_MD + 160, y, 50, VOICE_ROW_H, ::juce::Justification::centredLeft );

				// Remove button
				const auto removeR = ::juce::Rectangle<int>( getWidth() - PAD_MD - 16, y + ( VOICE_ROW_H - 16 ) / 2, 16, 16 );
				g.setColour( colours::removeBg );
				g.fillRoundedRectangle( removeR.toFloat(), BTN_CORNER_RADIUS );
				g.setColour( colours::removeAccent );
				g.drawText( "x", removeR, ::juce::Justification::centred );

				y += VOICE_ROW_H;
			}

			// Add voice button
			const auto addRect = ::juce::Rectangle<int>( PAD_MD, y + 2, getWidth() - 2 * PAD_MD, ADD_VOICE_H - 4 );
			g.setColour( colours::addBg );
			g.fillRoundedRectangle( addRect.toFloat(), BTN_CORNER_RADIUS );
			g.setColour( colours::addAccent );
			g.setFont( FONT_SM );
			g.drawText( "+ Add Voice", addRect, ::juce::Justification::centred );
		}

		void paintPreview( ::juce::Graphics &g, int y ) const {
			g.setColour( colours::rowAlt );
			g.fillRect( 0, y, getWidth(), PREVIEW_H );

			if ( mGroup.voices.empty() ) {
				g.setColour( colours::offWhite );
				g.setFont( FONT_SM );
				g.drawText( "(no voices generated)", PAD_MD, y, getWidth() - 2 * PAD_MD, PREVIEW_H, ::juce::Justification::centredLeft );
				return;
			}

			g.setFont( FONT_SM );
			::juce::String preview;
			for ( int i = 0; i < static_cast<int>( mGroup.voices.size() ); ++i ) {
				if ( i > 0 )
					preview += " -> ";
				if ( mGroup.mode == ::plop::PluginMode::Drums ) {
					const char *name = music::gmDrumName( mGroup.voices[ i ].pitch );
					preview += name ? ::juce::String( name ) : ::juce::String( mGroup.voices[ i ].pitch );
				} else {
					preview += groupPanelPitchName( mGroup.voices[ i ].pitch );
				}
			}

			g.setColour( ::juce::Colours::white );
			g.drawText( preview, PAD_MD, y, getWidth() - 2 * PAD_MD, PREVIEW_H / 2, ::juce::Justification::centredLeft );

			// Offsets line
			::juce::String offsets;
			for ( int i = 0; i < static_cast<int>( mGroup.voices.size() ); ++i ) {
				if ( i > 0 )
					offsets += ", ";
				offsets += ::juce::String( mGroup.voices[ i ].offset, 2 );
			}
			g.setColour( colours::offWhite );
			g.drawText(
			  "offsets: " + offsets, PAD_MD, y + PREVIEW_H / 2, getWidth() - 2 * PAD_MD, PREVIEW_H / 2, ::juce::Justification::centredLeft );
		}

		void handleControlsClick( ::juce::Point<int> pos, int /*y*/ ) {
			const int lx = PAD_MD;

			if ( mGroup.mode == ::plop::PluginMode::Drums ) {
				// Pattern field click -> cycle
				if ( pos.x >= lx + 26 && pos.x < lx + 96 ) {
					cyclePattern( music::drumsPatterns() );
					return;
				}
				// Note count drag
				if ( pos.x >= lx + 116 && pos.x < lx + 136 ) {
					startDrag( DragField::NoteCount, pos.y, mGroup.noteCount );
					return;
				}
			} else {
				// Root pitch drag
				if ( pos.x >= lx + 32 && pos.x < lx + 72 ) {
					startDrag( DragField::RootPitch, pos.y, mGroup.rootPitch );
					return;
				}
				// Pattern field click -> cycle
				if ( pos.x >= lx + 102 && pos.x < lx + 172 ) {
					const auto &pats =
					  mGroup.mode == ::plop::PluginMode::Silica ? music::silicaPatterns() : music::melodyPatterns();
					cyclePattern( pats );
					return;
				}
				// Note count drag
				if ( pos.x >= lx + 192 && pos.x < lx + 212 ) {
					startDrag( DragField::NoteCount, pos.y, mGroup.noteCount );
					return;
				}
			}

			// Period drag (in header area — handled via controls row period region)
			// Channel drag
		}

		void cyclePattern( const ::std::vector<PatternFunction> &patterns ) {
			if ( patterns.empty() )
				return;
			int idx = 0;
			for ( int i = 0; i < static_cast<int>( patterns.size() ); ++i ) {
				if ( patterns[ i ] == mGroup.pattern ) {
					idx = i;
					break;
				}
			}
			idx            = ( idx + 1 ) % static_cast<int>( patterns.size() );
			mGroup.pattern = patterns[ idx ];
			if ( onGroupChanged )
				onGroupChanged();
			repaint();
		}

		void startDrag( DragField field, int startY, int startVal ) {
			mDragField    = field;
			mDragStartY   = startY;
			mDragStartInt = startVal;
			setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
		}

		void startDrag( DragField field, int startY, float startVal ) {
			mDragField      = field;
			mDragStartY     = startY;
			mDragStartFloat = startVal;
			setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( GroupPanel )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_GROUP_PANEL_HPP
