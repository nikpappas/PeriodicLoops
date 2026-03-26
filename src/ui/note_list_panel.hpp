#ifndef PLOP_SRC_UI_NOTE_LIST_PANEL_HPP
#define PLOP_SRC_UI_NOTE_LIST_PANEL_HPP

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/drums.hpp"
#include "music/midi.hpp"

namespace plop::ui {

	namespace {
		inline ::juce::String midiPitchToName( int pitch ) {
			static constexpr const char *names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
			return ::juce::String( names[ pitch % 12 ] ) + ::juce::String( pitch / 12 - 1 );
		}
	} // namespace

	class NoteListPanel : public ::juce::Component {
	 public:
		enum class Mode { Pro, Melody, Drums };

		std::function<void( int, ::juce::Rectangle<int> )> onColourSwatchClicked;
		std::function<void( int )>                         onRemoveNote;
		std::function<void()>                              onAddNote;
		std::function<void( int, PeriodicNote )>           onNoteChanged;

		NoteListPanel() {
			mViewport.setScrollBarsShown( true, false );
			mViewport.setViewedComponent( &mRows, false );
			addAndMakeVisible( mViewport );

			mRows.onColourSwatchClicked = [ this ]( int i, ::juce::Rectangle<int> sb ) {
				if ( onColourSwatchClicked ) onColourSwatchClicked( i, sb );
			};
			mRows.onRemoveNote  = [ this ]( int i ) { if ( onRemoveNote ) onRemoveNote( i ); };
			mRows.onAddNote     = [ this ]() { if ( onAddNote ) onAddNote(); };
			mRows.onNoteChanged = [ this ]( int i, PeriodicNote n ) { if ( onNoteChanged ) onNoteChanged( i, n ); };
		}

		void setNotes( std::vector<PeriodicNote> notes ) {
			mRows.setNotes( std::move( notes ) );
			syncRowsSize();
		}

		void setColours( std::vector<::juce::Colour> colours ) {
			mRows.setColours( std::move( colours ) );
		}

		void setShowChannel( bool show ) {
			mRows.setShowChannel( show );
		}

		void setMode( Mode mode ) {
			mMode = mode;
			mRows.setMode( mode );
			repaint();
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( ::juce::Colour( 0xff12121f ) );

			g.setColour( ::juce::Colour( 0xff555577 ) );
			g.fillRect( 0, 0, getWidth(), k_header_h );
			g.setColour( ::juce::Colours::white );
			g.setFont( ::juce::Font( 13.0f, ::juce::Font::bold ) );
			g.drawText( "Notes", k_padding, 0, getWidth() - k_padding, k_header_h, ::juce::Justification::centredLeft );

			g.setColour( ::juce::Colour( 0xff888899 ) );
			g.setFont( 11.0f );
			const int y_cols = k_header_h + 4;
			g.drawText( "Colour",                                       k_padding,        y_cols, 18, 20, ::juce::Justification::centredLeft );
			g.drawText( mMode == Mode::Drums ? "Drum" : "Pitch",        k_padding + 22,   y_cols, 65, 20, ::juce::Justification::centredLeft );
			g.drawText( "Period",                                        k_padding + 91,   y_cols, 46, 20, ::juce::Justification::centredLeft );
			g.drawText( "Offset",                                        k_padding + 141,  y_cols, mRows.showChannel() ? 40 : 64, 20, ::juce::Justification::centredLeft );
			if ( mRows.showChannel() )
				g.drawText( "Ch", k_padding + 185, y_cols, 22, 20, ::juce::Justification::centredLeft );

			g.setColour( ::juce::Colour( 0xff333344 ) );
			g.drawHorizontalLine( k_total_header_h - 2, 0.0f, static_cast<float>( getWidth() ) );
		}

		void resized() override {
			mViewport.setBounds( 0, k_total_header_h, getWidth(), getHeight() - k_total_header_h );
			syncRowsSize();
		}

	 private:
		static constexpr int k_padding          = 8;
		static constexpr int k_header_h         = 30;
		static constexpr int k_total_header_h   = k_header_h + 4 + 22; // 56

		Mode mMode = Mode::Melody;

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

			void setNotes( std::vector<PeriodicNote> notes ) { mNotes = std::move( notes ); }
			void setColours( std::vector<::juce::Colour> c ) { mColours = std::move( c ); }
			void setShowChannel( bool show ) { mShowChannel = show; }
			bool showChannel() const { return mShowChannel; }
			void setMode( Mode m ) { mMode = m; }
			Mode mode() const { return mMode; }

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
					const auto sb     = swatchRect( i );
					const auto colour = colourFor( i );
					g.setColour( colour );
					g.fillRoundedRectangle( sb.toFloat(), 3.0f );
					g.setColour( colour.brighter( 0.3f ) );
					g.drawRoundedRectangle( sb.toFloat(), 3.0f, 1.0f );

					g.setColour( ::juce::Colours::white );
					drawCell( g, pitchLabel( note.pitch ),              pitchRect( i ),   i == mEditingIndex && mEditingField == Field::Pitch );
					drawCell( g, ::juce::String( note.period, 2 ) + " b", periodRect( i ),  i == mEditingIndex && mEditingField == Field::Period );
					drawCell( g, ::juce::String( note.offset, 2 ) + " b", offsetRect( i ),  i == mEditingIndex && mEditingField == Field::Offset );
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
				const auto addB = ::juce::Rectangle<int>{ k_padding, addY, getWidth() - 2 * k_padding, 22 };
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
					if ( onRemoveNote && removeRect( i ).contains( pos ) ) { onRemoveNote( i ); return; }

					Field f = Field::None;
					if ( pitchRect( i ).contains( pos ) )                          f = Field::Pitch;
					else if ( periodRect( i ).contains( pos ) )                    f = Field::Period;
					else if ( offsetRect( i ).contains( pos ) )                    f = Field::Offset;
					else if ( mShowChannel && channelRect( i ).contains( pos ) )   f = Field::Channel;

					if ( f != Field::None ) {
						mDragIndex      = i;
						mDragField      = f;
						mDragStartY    = pos.y;
						mDragStartNote = mNotes[ i ];
						setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
						return;
					}
				}
				const int  addY = static_cast<int>( mNotes.size() ) * row_h + 6;
				const auto addB = ::juce::Rectangle<int>{ k_padding, addY, getWidth() - 2 * k_padding, 22 };
				if ( onAddNote && addB.contains( pos ) ) onAddNote();
			}

			void mouseDrag( const ::juce::MouseEvent &e ) override {
				if ( mDragIndex < 0 || mDragField == Field::None ) return;
				const int dy = mDragStartY - e.getPosition().y; // up = positive = increase

				PeriodicNote updated = mDragStartNote;
				if ( mDragField == Field::Pitch ) {
					if ( mMode == Mode::Drums ) {
						const int startIdx = music::gmDrumIndexForNote( mDragStartNote.pitch );
						const int newIdx   = ::juce::jlimit( 0, static_cast<int>( music::k_gmDrums.size() ) - 1, startIdx + dy / 4 );
						updated.pitch      = music::gmDrumNoteAtIndex( newIdx );
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
				if ( onNoteChanged ) onNoteChanged( mDragIndex, updated );
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
					if ( pitchRect( i ).contains( e.getPosition() ) )   { startEdit( i, Field::Pitch );   return; }
					if ( periodRect( i ).contains( e.getPosition() ) )  { startEdit( i, Field::Period );  return; }
					if ( offsetRect( i ).contains( e.getPosition() ) )  { startEdit( i, Field::Offset );  return; }
					if ( mShowChannel && channelRect( i ).contains( e.getPosition() ) ) { startEdit( i, Field::Channel ); return; }
				}
			}

		 private:
			enum class Field { None, Pitch, Period, Offset, Channel };
			static constexpr int k_padding = 8;
			static constexpr int swatch_w  = 18;
			static constexpr int swatch_h  = 14;
			static constexpr int row_h     = 28;

			std::vector<PeriodicNote>   mNotes;
			std::vector<::juce::Colour> mColours;
			::juce::TextEditor          mEditor;
			int                         mEditingIndex = -1;
			Field                       mEditingField = Field::None;
			bool                        mShowChannel  = false;
			Mode                        mMode         = Mode::Melody;

			int          mDragIndex      = -1;
			Field        mDragField      = Field::None;
			int          mDragStartY    = 0;
			PeriodicNote mDragStartNote = {};

			::juce::Rectangle<int> swatchRect( int i ) const {
				return { k_padding, i * row_h + ( row_h - swatch_h ) / 2, swatch_w, swatch_h };
			}
			::juce::Rectangle<int> pitchRect( int i ) const  { return { k_padding + 22,  i * row_h, 65, row_h }; }
			::juce::Rectangle<int> periodRect( int i ) const { return { k_padding + 91,  i * row_h, 46, row_h }; }
			::juce::Rectangle<int> offsetRect( int i ) const {
				return { k_padding + 141, i * row_h, mShowChannel ? 40 : 64, row_h };
			}
			::juce::Rectangle<int> channelRect( int i ) const { return { k_padding + 185, i * row_h, 22, row_h }; }
			::juce::Rectangle<int> removeRect( int i ) const {
				return { getWidth() - k_padding - 16, i * row_h + ( row_h - 16 ) / 2, 16, 16 };
			}

			::juce::Colour colourFor( int i ) const {
				return i < static_cast<int>( mColours.size() ) ? mColours[ i ] : ::juce::Colours::grey;
			}

			::juce::String pitchLabel( int pitch ) const {
				if ( mMode == Mode::Drums ) {
					const char *name = music::gmDrumName( pitch );
					return name ? ::juce::String( name ) : ::juce::String( pitch );
				}
				if ( mMode == Mode::Pro )
					return ::juce::String( pitch );
				return midiPitchToName( pitch );
			}

			void drawCell( ::juce::Graphics &g, const ::juce::String &text,
			               ::juce::Rectangle<int> bounds, bool isActive ) const {
				if ( isActive ) {
					g.setColour( ::juce::Colour( 0xff2a2a44 ) );
					g.fillRect( bounds );
				}
				g.setColour( ::juce::Colours::white );
				g.drawText( text, bounds, ::juce::Justification::centredLeft );
			}

			void startEdit( int i, Field field ) {
				mEditingIndex = i;
				mEditingField = field;
				::juce::Rectangle<int> bounds;
				::juce::String         text;
				if ( field == Field::Pitch ) {
					bounds = pitchRect( i );   text = ::juce::String( mNotes[ i ].pitch );
					mEditor.setInputRestrictions( 3, "0123456789" );
				} else if ( field == Field::Period ) {
					bounds = periodRect( i );  text = ::juce::String( mNotes[ i ].period );
					mEditor.setInputRestrictions( 8, "0123456789." );
				} else if ( field == Field::Offset ) {
					bounds = offsetRect( i );  text = ::juce::String( mNotes[ i ].offset );
					mEditor.setInputRestrictions( 8, "0123456789." );
				} else {
					bounds = channelRect( i ); text = ::juce::String( mNotes[ i ].channel );
					mEditor.setInputRestrictions( 2, "0123456789" );
				}
				mEditor.setBounds( bounds.reduced( 2 ) );
				mEditor.setText( text, false );
				mEditor.setVisible( true );
				mEditor.grabKeyboardFocus();
				mEditor.selectAll();
			}

			void commitEdit() {
				if ( mEditingIndex < 0 || mEditingField == Field::None ) return;
				PeriodicNote updated = mNotes[ mEditingIndex ];
				if ( mEditingField == Field::Pitch )
					updated.pitch = ::juce::jlimit( 0, 127, mEditor.getText().getIntValue() );
				else if ( mEditingField == Field::Period )
					updated.period = ::juce::jmax( 0.01f, mEditor.getText().getFloatValue() );
				else if ( mEditingField == Field::Offset )
					updated.offset = ::juce::jlimit( 0.0f, updated.period, mEditor.getText().getFloatValue() );
				else
					updated.channel = ::juce::jlimit( 0, 15, mEditor.getText().getIntValue() );
				if ( onNoteChanged ) onNoteChanged( mEditingIndex, updated );
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
