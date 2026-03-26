#ifndef PLOP_SRC_UI_NOTE_LIST_PANEL_HPP
#define PLOP_SRC_UI_NOTE_LIST_PANEL_HPP

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/midi.hpp"

namespace plop::ui {

	class NoteListPanel : public ::juce::Component {
	 public:
		std::function<void( int index, ::juce::Rectangle<int> screenBounds )> onColourSwatchClicked;
		std::function<void( int index )>                                       onRemoveNote;
		std::function<void()>                                                  onAddNote;
		/// Fired when any editable field changes. The full updated note is provided.
		std::function<void( int index, PeriodicNote note )>                    onNoteChanged;

		NoteListPanel() {
			m_editor.setJustification( ::juce::Justification::centred );
			m_editor.setColour( ::juce::TextEditor::backgroundColourId, ::juce::Colour( 0xff2a2a44 ) );
			m_editor.setColour( ::juce::TextEditor::textColourId, ::juce::Colours::white );
			m_editor.setColour( ::juce::TextEditor::outlineColourId, ::juce::Colour( 0xff4fc3f7 ) );
			m_editor.onReturnKey = [this] { commitEdit(); };
			m_editor.onEscapeKey = [this] { cancelEdit(); };
			m_editor.onFocusLost = [this] { commitEdit(); };
			addChildComponent( m_editor );
		}

		void setNotes( std::vector<PeriodicNote> notes ) {
			m_notes = std::move( notes );
		}

		void setColours( std::vector<::juce::Colour> colours ) {
			m_colours = std::move( colours );
		}

		void setShowChannel( bool show ) {
			m_show_channel = show;
		}

		int getContentHeight() const {
			return listTop() + static_cast<int>( m_notes.size() ) * row_h + 6 + 22 + 8;
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( ::juce::Colour( 0xff12121f ) );

			// Header
			g.setColour( ::juce::Colour( 0xff555577 ) );
			g.fillRect( 0, 0, getWidth(), header_h );
			g.setColour( ::juce::Colours::white );
			g.setFont( ::juce::Font( 13.0f, ::juce::Font::bold ) );
			g.drawText( "Notes", padding, 0, getWidth() - padding, header_h, ::juce::Justification::centredLeft );

			// Column headers
			g.setColour( ::juce::Colour( 0xff888899 ) );
			g.setFont( 11.0f );
			const int y_cols = header_h + 4;
			g.drawText( "Colour", padding,       y_cols, swatch_w, 20, ::juce::Justification::centredLeft );
			g.drawText( "Pitch",  padding + 30,  y_cols, 40,       20, ::juce::Justification::centredLeft );
			g.drawText( "Period", padding + 75,  y_cols, 55,       20, ::juce::Justification::centredLeft );
			g.drawText( "Offset", padding + 135, y_cols, m_show_channel ? 45 : 65, 20, ::juce::Justification::centredLeft );
			if ( m_show_channel )
				g.drawText( "Ch", padding + 182, y_cols, 22, 20, ::juce::Justification::centredLeft );

			// Divider
			const int list_top = listTop();
			g.setColour( ::juce::Colour( 0xff333344 ) );
			g.drawHorizontalLine( list_top - 2, 0.0f, static_cast<float>( getWidth() ) );

			// Rows
			g.setFont( 13.0f );
			for ( int i = 0; i < static_cast<int>( m_notes.size() ); ++i ) {
				const auto &note = m_notes[ i ];
				const int   y    = list_top + i * row_h;

				if ( i % 2 == 0 ) {
					g.setColour( ::juce::Colour( 0xff1e1e30 ) );
					g.fillRect( 0, y, getWidth(), row_h );
				}

				// Colour swatch
				const auto sb     = swatchRect( i, list_top );
				const auto colour = colourFor( i );
				g.setColour( colour );
				g.fillRoundedRectangle( sb.toFloat(), 3.0f );
				g.setColour( colour.brighter( 0.3f ) );
				g.drawRoundedRectangle( sb.toFloat(), 3.0f, 1.0f );

				// Editable fields — highlight on hover hint via brighter text
				g.setColour( ::juce::Colours::white );
				drawEditableCell( g, ::juce::String( note.pitch ),            pitchRect( i, list_top ),   i == m_editing_index && m_editing_field == Field::Pitch );
				drawEditableCell( g, ::juce::String( note.period, 2 ) + " b", periodRect( i, list_top ),  i == m_editing_index && m_editing_field == Field::Period );
				drawEditableCell( g, ::juce::String( note.offset, 2 ) + " b", offsetRect( i, list_top ),  i == m_editing_index && m_editing_field == Field::Offset );
				if ( m_show_channel )
					drawEditableCell( g, ::juce::String( note.channel ), channelRect( i, list_top ), i == m_editing_index && m_editing_field == Field::Channel );

				// Remove button
				const auto rb = removeRect( i, list_top );
				g.setColour( ::juce::Colour( 0xff553333 ) );
				g.fillRoundedRectangle( rb.toFloat(), 3.0f );
				g.setColour( ::juce::Colour( 0xffff6666 ) );
				g.setFont( 11.0f );
				g.drawText( "x", rb, ::juce::Justification::centred );
			}

			// Add button
			const int  addY = list_top + static_cast<int>( m_notes.size() ) * row_h + 6;
			const auto addB = ::juce::Rectangle<int>{ padding, addY, getWidth() - 2 * padding, 22 };
			g.setColour( ::juce::Colour( 0xff223322 ) );
			g.fillRoundedRectangle( addB.toFloat(), 4.0f );
			g.setColour( ::juce::Colour( 0xff66cc66 ) );
			g.setFont( 13.0f );
			g.drawText( "+ Add Note", addB, ::juce::Justification::centred );
		}

		void mouseDown( const ::juce::MouseEvent &e ) override {
			const int list_top = listTop();

			for ( int i = 0; i < static_cast<int>( m_notes.size() ); ++i ) {
				if ( onColourSwatchClicked && swatchRect( i, list_top ).contains( e.getPosition() ) ) {
					onColourSwatchClicked( i, localAreaToGlobal( swatchRect( i, list_top ) ) );
					return;
				}
				if ( pitchRect( i, list_top ).contains( e.getPosition() ) ) {
					startEdit( i, Field::Pitch, list_top );
					return;
				}
				if ( periodRect( i, list_top ).contains( e.getPosition() ) ) {
					startEdit( i, Field::Period, list_top );
					return;
				}
				if ( offsetRect( i, list_top ).contains( e.getPosition() ) ) {
					startEdit( i, Field::Offset, list_top );
					return;
				}
				if ( m_show_channel && channelRect( i, list_top ).contains( e.getPosition() ) ) {
					startEdit( i, Field::Channel, list_top );
					return;
				}
				if ( onRemoveNote && removeRect( i, list_top ).contains( e.getPosition() ) ) {
					onRemoveNote( i );
					return;
				}
			}

			const int  addY = list_top + static_cast<int>( m_notes.size() ) * row_h + 6;
			const auto addB = ::juce::Rectangle<int>{ padding, addY, getWidth() - 2 * padding, 22 };
			if ( onAddNote && addB.contains( e.getPosition() ) )
				onAddNote();
		}

	 private:
		enum class Field { None, Pitch, Period, Offset, Channel };

		static constexpr int swatch_w = 18;
		static constexpr int swatch_h = 14;
		static constexpr int padding  = 8;
		static constexpr int header_h = 30;
		static constexpr int row_h    = 28;

		std::vector<PeriodicNote>   m_notes;
		std::vector<::juce::Colour> m_colours;
		::juce::TextEditor          m_editor;
		int                         m_editing_index = -1;
		Field                       m_editing_field = Field::None;
		bool                        m_show_channel  = false;

		int listTop() const { return header_h + 4 + 22; }

		::juce::Rectangle<int> swatchRect( int i, int lt ) const {
			return { padding, lt + i * row_h + ( row_h - swatch_h ) / 2, swatch_w, swatch_h };
		}
		::juce::Rectangle<int> pitchRect( int i, int lt ) const {
			return { padding + 30, lt + i * row_h, 40, row_h };
		}
		::juce::Rectangle<int> periodRect( int i, int lt ) const {
			return { padding + 75, lt + i * row_h, 55, row_h };
		}
		::juce::Rectangle<int> offsetRect( int i, int lt ) const {
			return { padding + 135, lt + i * row_h, m_show_channel ? 45 : 65, row_h };
		}
		::juce::Rectangle<int> channelRect( int i, int lt ) const {
			return { padding + 182, lt + i * row_h, 22, row_h };
		}
		::juce::Rectangle<int> removeRect( int i, int lt ) const {
			return { getWidth() - padding - 16, lt + i * row_h + ( row_h - 16 ) / 2, 16, 16 };
		}

		::juce::Colour colourFor( int i ) const {
			return i < static_cast<int>( m_colours.size() ) ? m_colours[ i ] : ::juce::Colours::grey;
		}

		void drawEditableCell( ::juce::Graphics &g, const ::juce::String &text,
		                       ::juce::Rectangle<int> bounds, bool isActive ) const {
			if ( isActive ) {
				g.setColour( ::juce::Colour( 0xff2a2a44 ) );
				g.fillRect( bounds );
			}
			g.setColour( ::juce::Colours::white );
			g.drawText( text, bounds, ::juce::Justification::centredLeft );
		}

		void startEdit( int i, Field field, int list_top ) {
			m_editing_index = i;
			m_editing_field = field;

			::juce::Rectangle<int> bounds;
			::juce::String         text;

			if ( field == Field::Pitch ) {
				bounds = pitchRect( i, list_top );
				text   = ::juce::String( m_notes[ i ].pitch );
				m_editor.setInputRestrictions( 3, "0123456789" );
			} else if ( field == Field::Period ) {
				bounds = periodRect( i, list_top );
				text   = ::juce::String( m_notes[ i ].period );
				m_editor.setInputRestrictions( 8, "0123456789." );
			} else if ( field == Field::Offset ) {
				bounds = offsetRect( i, list_top );
				text   = ::juce::String( m_notes[ i ].offset );
				m_editor.setInputRestrictions( 8, "0123456789." );
			} else {
				bounds = channelRect( i, list_top );
				text   = ::juce::String( m_notes[ i ].channel );
				m_editor.setInputRestrictions( 2, "0123456789" );
			}

			m_editor.setBounds( bounds.reduced( 2 ) );
			m_editor.setText( text, false );
			m_editor.setVisible( true );
			m_editor.grabKeyboardFocus();
			m_editor.selectAll();
		}

		void commitEdit() {
			if ( m_editing_index < 0 || m_editing_field == Field::None ) return;

			PeriodicNote updated = m_notes[ m_editing_index ];

			if ( m_editing_field == Field::Pitch ) {
				updated.pitch = ::juce::jlimit( 0, 127, m_editor.getText().getIntValue() );
			} else if ( m_editing_field == Field::Period ) {
				updated.period = ::juce::jmax( 0.01f, m_editor.getText().getFloatValue() );
			} else if ( m_editing_field == Field::Offset ) {
				updated.offset = ::juce::jmax( 0.0f, m_editor.getText().getFloatValue() );
			} else {
				updated.channel = ::juce::jlimit( 0, 15, m_editor.getText().getIntValue() );
			}

			if ( onNoteChanged ) onNoteChanged( m_editing_index, updated );
			cancelEdit();
		}

		void cancelEdit() {
			m_editor.setVisible( false );
			m_editing_index = -1;
			m_editing_field = Field::None;
		}
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_NOTE_LIST_PANEL_HPP
