#ifndef PLOP_SRC_UI_NOTE_LIST_PANEL_HPP
#define PLOP_SRC_UI_NOTE_LIST_PANEL_HPP

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/drums.hpp"
#include "music/midi.hpp"
#include "music/scales.hpp"
#include "processor/engine.hpp"

namespace plop::ui {

	namespace {
		inline ::juce::String midiPitchToName( int pitch ) {
			static constexpr const char *names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
			return ::juce::String( names[ pitch % 12 ] ) + ::juce::String( pitch / 12 - 2 );
		}
	} // namespace

	class NoteListPanel : public ::juce::Component {
	 public:
		struct Callbacks {
			std::function<void( int, ::juce::Rectangle<int> )> onColourSwatchClicked;
			std::function<void( int )>                         onRemoveNote;
			std::function<void()>                              onAddNote;
			std::function<void( int, PeriodicNote )>           onNoteChanged;
		};

		explicit NoteListPanel( Callbacks cbs ) : mCbs( std::move( cbs ) ) {
			mViewport.setScrollBarsShown( true, false );
			mViewport.setViewedComponent( &mRows, false );
			addAndMakeVisible( mViewport );

			mRows.onColourSwatchClicked = [ this ]( int i, ::juce::Rectangle<int> sb ) {
				if ( mCbs.onColourSwatchClicked )
					mCbs.onColourSwatchClicked( i, sb );
			};
			mRows.onRemoveNote = [ this ]( int i ) {
				if ( mCbs.onRemoveNote )
					mCbs.onRemoveNote( i );
			};
			mRows.onAddNote = [ this ]() {
				if ( mCbs.onAddNote )
					mCbs.onAddNote();
			};
			mRows.onNoteChanged = [ this ]( int i, PeriodicNote n ) {
				if ( mCbs.onNoteChanged )
					mCbs.onNoteChanged( i, n );
			};
		}

		void setNotes( ::std::vector<PeriodicNote> notes ) {
			mRows.setNotes( std::move( notes ) );
			syncRowsSize();
		}

		void setColours( ::std::vector<::juce::Colour> colours ) {
			mRows.setColours( std::move( colours ) );
		}

		void setShowChannel( bool show ) {
			mRows.setShowChannel( show );
		}

		void setMode( PluginMode mode ) {
			mMode = mode;
			mRows.setMode( mode );
			repaint();
		}

		void setScaleConstraint( int root, int scaleTypeIndex ) {
			mRows.setScaleConstraint( root, scaleTypeIndex );
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( ::juce::Colour( 0xff12121f ) );

			g.setColour( ::juce::Colour( 0xff555577 ) );
			g.fillRect( 0, 0, getWidth(), kHeaderH );
			g.setColour( ::juce::Colours::white );
			g.setFont( ::juce::Font( 13.0f, ::juce::Font::bold ) );
			g.drawText( "Notes", kPadding, 0, getWidth() - kPadding, kHeaderH, ::juce::Justification::centredLeft );

			g.setColour( ::juce::Colour( 0xff888899 ) );
			g.setFont( 11.0f );
			const int y_cols = kHeaderH + 4;
			g.drawText( mMode == PluginMode::Drums ? "Drum" : "Pitch", kPadding + 22, y_cols, 65, 20, ::juce::Justification::centredLeft );
			g.drawText( "Period", kPadding + 111, y_cols, 46, 20, ::juce::Justification::centredLeft );
			g.drawText( "Offset", kPadding + 161, y_cols, mRows.showChannel() ? 40 : 64, 20, ::juce::Justification::centredLeft );
			if ( mRows.showChannel() )
				g.drawText( "Ch", kPadding + 205, y_cols, 22, 20, ::juce::Justification::centredLeft );

			g.setColour( ::juce::Colour( 0xff333344 ) );
			g.drawHorizontalLine( kTotalHeaderH - 2, 0.0f, static_cast<float>( getWidth() ) );
		}

		void resized() override {
			mViewport.setBounds( 0, kTotalHeaderH, getWidth(), getHeight() - kTotalHeaderH );
			syncRowsSize();
		}

	 private:
		static constexpr int kPadding      = 8;
		static constexpr int kHeaderH      = 30;
		static constexpr int kTotalHeaderH = kHeaderH + 4 + 22; // 56

		const Callbacks mCbs;
		PluginMode      mMode = PluginMode::Melody;

		void syncRowsSize() {
			if ( mViewport.getWidth() > 0 )
				mRows.setSize( mViewport.getWidth(), mRows.getContentHeight() );
		}

		// ---- scrollable rows -----------------------------------------------
		class RowsComponent : public ::juce::Component {
		 public:
			std::function<void( int, ::juce::Rectangle<int> )> onColourSwatchClicked;
			std::function<void( int )>                         onRemoveNote;
			std::function<void()>                              onAddNote;
			std::function<void( int, PeriodicNote )>           onNoteChanged;

			RowsComponent() {
				mEditor.setJustification( ::juce::Justification::centred );
				mEditor.setColour( ::juce::TextEditor::backgroundColourId, ::juce::Colour( 0xff2a2a44 ) );
				mEditor.setColour( ::juce::TextEditor::textColourId, ::juce::Colours::white );
				mEditor.setColour( ::juce::TextEditor::outlineColourId, ::juce::Colour( 0xff4fc3f7 ) );
				mEditor.onReturnKey = [ this ] { commitEdit(); };
				mEditor.onEscapeKey = [ this ] { cancelEdit(); };
				mEditor.onFocusLost = [ this ] { commitEdit(); };
				addChildComponent( mEditor );
			}

			void setNotes( std::vector<PeriodicNote> notes ) {
				mNotes = std::move( notes );
			}
			void setColours( std::vector<::juce::Colour> c ) {
				mColours = std::move( c );
			}

			void setShowChannel( bool show ) {
				mShowChannel = show;
			}

			bool showChannel() const {
				return mShowChannel;
			}

			void setMode( PluginMode m ) {
				mMode = m;
			}

			PluginMode mode() const {
				return mMode;
			}
			void setScaleConstraint( int root, int scaleTypeIndex ) {
				mScaleRoot = root;
				mScaleType = scaleTypeIndex;
			}
			int getContentHeight() const {
				return static_cast<int>( mNotes.size() ) * row_h + 6 + 22 + 8;
			}

			void paint( ::juce::Graphics &g ) override {
				g.fillAll( ::juce::Colour( 0xff12121f ) );
				g.setFont( 13.0f );
				for ( int i = 0; i < static_cast<int>( mNotes.size() ); ++i ) {
					const auto &note = mNotes[ i ];
					const int   y    = i * row_h;
					if ( i % 2 == 0 ) {
						g.setColour( ::juce::Colour( 0xff1e1e30 ) );
						g.fillRect( 0, y, getWidth(), row_h );
					}
					const auto sb                 = swatchRect( i );
					const auto colour             = colourFor( i );
					const auto swatchCornerRadius = swatch_s / 2.0f;
					g.setColour( colour );
					g.fillRoundedRectangle( sb.toFloat(), swatchCornerRadius );
					g.setColour( colour.brighter( 0.3f ) );
					g.drawRoundedRectangle( sb.toFloat(), swatchCornerRadius, 1.0f );

					const bool silica = ( mMode == PluginMode::Silica );
					g.setColour( ::juce::Colours::white );
					drawCell( g, pitchLabel( note.pitch ), pitchRect( i ), i == mEditingIndex && mEditingField == Field::Pitch );
					drawCell( g,
					          ::juce::String( note.period, 2 ) + " b",
					          periodRect( i ),
					          i == mEditingIndex && mEditingField == Field::Period,
					          silica );
					drawCell( g,
					          ::juce::String( note.offset, 2 ) + " b",
					          offsetRect( i ),
					          i == mEditingIndex && mEditingField == Field::Offset,
					          silica );
					if ( mShowChannel )
						drawCell( g, ::juce::String( note.channel ), channelRect( i ), i == mEditingIndex && mEditingField == Field::Channel );

					const auto rb = removeRect( i );
					g.setColour( ::juce::Colour( 0xff553333 ) );
					g.fillRoundedRectangle( rb.toFloat(), 3.0f );
					g.setColour( ::juce::Colour( 0xffff6666 ) );
					g.setFont( 11.0f );
					g.drawText( "x", rb, ::juce::Justification::centred );
					g.setFont( 13.0f );
				}
				const int  addY = static_cast<int>( mNotes.size() ) * row_h + 6;
				const auto addB = ::juce::Rectangle<int>{ kPadding, addY, getWidth() - 2 * kPadding, 22 };
				g.setColour( ::juce::Colour( 0xff223322 ) );
				g.fillRoundedRectangle( addB.toFloat(), 4.0f );
				g.setColour( ::juce::Colour( 0xff66cc66 ) );
				g.setFont( 13.0f );
				g.drawText( "+ Add Note", addB, ::juce::Justification::centred );
			}

			void mouseDown( const ::juce::MouseEvent &e ) override {
				const auto pos = e.getPosition();
				for ( int i = 0; i < static_cast<int>( mNotes.size() ); ++i ) {
					if ( onColourSwatchClicked && swatchRect( i ).contains( pos ) ) {
						onColourSwatchClicked( i, localAreaToGlobal( swatchRect( i ) ) );
						return;
					}
					if ( onRemoveNote && removeRect( i ).contains( pos ) ) {
						onRemoveNote( i );
						return;
					}

					Field f = Field::None;
					if ( pitchRect( i ).contains( pos ) )
						f = Field::Pitch;
					else if ( !( mMode == PluginMode::Silica ) && periodRect( i ).contains( pos ) )
						f = Field::Period;
					else if ( !( mMode == PluginMode::Silica ) && offsetRect( i ).contains( pos ) )
						f = Field::Offset;
					else if ( mShowChannel && channelRect( i ).contains( pos ) )
						f = Field::Channel;

					if ( f != Field::None ) {
						mDragIndex     = i;
						mDragField     = f;
						mDragStartY    = pos.y;
						mDragStartNote = mNotes[ i ];
						setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
						return;
					}
				}
				const int  addY = static_cast<int>( mNotes.size() ) * row_h + 6;
				const auto addB = ::juce::Rectangle<int>{ kPadding, addY, getWidth() - 2 * kPadding, 22 };
				if ( onAddNote && addB.contains( pos ) )
					onAddNote();
			}

			void mouseDrag( const ::juce::MouseEvent &e ) override {
				if ( mDragIndex < 0 || mDragField == Field::None )
					return;
				const int dy = mDragStartY - e.getPosition().y; // up = positive = increase

				PeriodicNote updated = mDragStartNote;
				if ( mDragField == Field::Pitch ) {
					if ( mMode == PluginMode::Drums ) {
						const int startIdx = music::gmDrumIndexForNote( mDragStartNote.pitch );
						const int newIdx = ::juce::jlimit( 0, static_cast<int>( music::kGmDrums.size() ) - 1, startIdx + dy / 4 );
						updated.pitch = music::gmDrumNoteAtIndex( newIdx );
					} else if ( mMode == PluginMode::Scale ) {
						const auto &pc    = music::k_scales[ static_cast<size_t>( mScaleType ) ].pitchClasses;
						const int   steps = dy / 4;
						int         p     = mDragStartNote.pitch;
						for ( int s = 0; s < std::abs( steps ); ++s )
							p = music::stepInScale( p, steps > 0 ? 1 : -1, mScaleRoot, pc );
						updated.pitch = p;
					} else {
						updated.pitch = ::juce::jlimit( 0, 127, mDragStartNote.pitch + dy / 3 );
					}
				} else if ( mDragField == Field::Period ) {
					updated.period = ::juce::jmax( 0.01f, mDragStartNote.period + dy * 0.05f );
				} else if ( mDragField == Field::Offset ) {
					updated.offset = ::juce::jlimit( 0.0f, updated.period, mDragStartNote.offset + dy * 0.05f );
				} else {
					updated.channel = ::juce::jlimit( 0, 15, mDragStartNote.channel + dy / 8 );
				}

				mNotes[ mDragIndex ] = updated;
				if ( onNoteChanged )
					onNoteChanged( mDragIndex, updated );
				repaint();
			}

			void mouseUp( const ::juce::MouseEvent & ) override {
				if ( mDragField != Field::None )
					setMouseCursor( ::juce::MouseCursor::NormalCursor );
				mDragIndex = -1;
				mDragField = Field::None;
			}

			void mouseDoubleClick( const ::juce::MouseEvent &e ) override {
				for ( int i = 0; i < static_cast<int>( mNotes.size() ); ++i ) {
					if ( pitchRect( i ).contains( e.getPosition() ) ) {
						startEdit( i, Field::Pitch );
						return;
					}
					if ( !( mMode == PluginMode::Silica ) && periodRect( i ).contains( e.getPosition() ) ) {
						startEdit( i, Field::Period );
						return;
					}
					if ( !( mMode == PluginMode::Silica ) && offsetRect( i ).contains( e.getPosition() ) ) {
						startEdit( i, Field::Offset );
						return;
					}
					if ( mShowChannel && channelRect( i ).contains( e.getPosition() ) ) {
						startEdit( i, Field::Channel );
						return;
					}
				}
			}

		 private:
			enum class Field { None, Pitch, Period, Offset, Channel };
			static constexpr int kPadding = 8;
			static constexpr int swatch_s = 14;
			static constexpr int row_h    = 28;

			std::vector<PeriodicNote>   mNotes;
			std::vector<::juce::Colour> mColours;
			::juce::TextEditor          mEditor;
			int                         mEditingIndex = -1;
			Field                       mEditingField = Field::None;
			bool                        mShowChannel  = false;
			PluginMode                  mMode         = PluginMode::Melody;
			int                         mScaleRoot    = 0;
			int                         mScaleType    = 1; // Major

			int          mDragIndex     = -1;
			Field        mDragField     = Field::None;
			int          mDragStartY    = 0;
			PeriodicNote mDragStartNote = {};

			::juce::Rectangle<int> swatchRect( int i ) const {
				return { kPadding, i * row_h + ( row_h - swatch_s ) / 2, swatch_s, swatch_s };
			}
			::juce::Rectangle<int> pitchRect( int i ) const {
				return { kPadding + 22, i * row_h, 85, row_h };
			}
			::juce::Rectangle<int> periodRect( int i ) const {
				return { kPadding + 111, i * row_h, 46, row_h };
			}
			::juce::Rectangle<int> offsetRect( int i ) const {
				return { kPadding + 161, i * row_h, mShowChannel ? 40 : 64, row_h };
			}
			::juce::Rectangle<int> channelRect( int i ) const {
				return { kPadding + 205, i * row_h, 22, row_h };
			}
			::juce::Rectangle<int> removeRect( int i ) const {
				return { getWidth() - kPadding - 16, i * row_h + ( row_h - 16 ) / 2, 16, 16 };
			}

			::juce::Colour colourFor( int i ) const {
				return i < static_cast<int>( mColours.size() ) ? mColours[ i ] : ::juce::Colours::grey;
			}

			::juce::String pitchLabel( int pitch ) const {
				if ( mMode == PluginMode::Drums ) {
					const char *name = music::gmDrumName( pitch );
					return name ? "(" + ::juce::String( pitch ) + ") " + ::juce::String( name ) : ::juce::String( pitch );
				}
				if ( mMode == PluginMode::Pro )
					return ::juce::String( pitch );
				// Melody, Silica, and Scale all show note names
				return "(" + ::juce::String( pitch ) + ") " + midiPitchToName( pitch );
			}

			void drawCell( ::juce::Graphics      &g,
			               const ::juce::String  &text,
			               ::juce::Rectangle<int> bounds,
			               bool                   isActive,
			               bool                   dimmed = false ) const {
				if ( isActive ) {
					g.setColour( ::juce::Colour( 0xff2a2a44 ) );
					g.fillRect( bounds );
				}
				g.setColour( dimmed ? ::juce::Colours::white.withAlpha( 0.35f ) : ::juce::Colours::white );
				g.drawText( text, bounds, ::juce::Justification::centredLeft );
			}

			void startEdit( int i, Field field ) {
				mEditingIndex = i;
				mEditingField = field;
				::juce::Rectangle<int> bounds;
				::juce::String         text;
				if ( field == Field::Pitch ) {
					bounds = pitchRect( i );
					text   = ::juce::String( mNotes[ i ].pitch );
					mEditor.setInputRestrictions( 3, "0123456789" );
				} else if ( field == Field::Period ) {
					bounds = periodRect( i );
					text   = ::juce::String( mNotes[ i ].period );
					mEditor.setInputRestrictions( 8, "0123456789." );
				} else if ( field == Field::Offset ) {
					bounds = offsetRect( i );
					text   = ::juce::String( mNotes[ i ].offset );
					mEditor.setInputRestrictions( 8, "0123456789." );
				} else {
					bounds = channelRect( i );
					text   = ::juce::String( mNotes[ i ].channel );
					mEditor.setInputRestrictions( 2, "0123456789" );
				}
				mEditor.setBounds( bounds.reduced( 2 ) );
				mEditor.setText( text, false );
				mEditor.setVisible( true );
				mEditor.grabKeyboardFocus();
				mEditor.selectAll();
			}

			void commitEdit() {
				if ( mEditingIndex < 0 || mEditingField == Field::None )
					return;
				PeriodicNote updated = mNotes[ mEditingIndex ];
				if ( mEditingField == Field::Pitch ) {
					int pitch = ::juce::jlimit( 0, 127, mEditor.getText().getIntValue() );
					if ( mMode == PluginMode::Scale )
						pitch = music::snapToScale( pitch, mScaleRoot, music::k_scales[ static_cast<size_t>( mScaleType ) ].pitchClasses );
					updated.pitch = pitch;
				} else if ( mEditingField == Field::Period )
					updated.period = ::juce::jmax( 0.01f, mEditor.getText().getFloatValue() );
				else if ( mEditingField == Field::Offset )
					updated.offset = ::juce::jlimit( 0.0f, updated.period, mEditor.getText().getFloatValue() );
				else
					updated.channel = ::juce::jlimit( 0, 15, mEditor.getText().getIntValue() );
				if ( onNoteChanged )
					onNoteChanged( mEditingIndex, updated );
				cancelEdit();
			}

			void cancelEdit() {
				mEditor.setVisible( false );
				mEditingIndex = -1;
				mEditingField = Field::None;
			}

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( RowsComponent )
		};

		RowsComponent    mRows;
		::juce::Viewport mViewport;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( NoteListPanel )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_NOTE_LIST_PANEL_HPP
