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

		void setNotes( std::vector<PeriodicNote> notes ) {
			m_notes = std::move( notes );
		}

		void setColours( std::vector<::juce::Colour> colours ) {
			m_colours = std::move( colours );
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
			g.drawText( "Colour", padding, y_cols, swatch_w, 20, ::juce::Justification::centredLeft );
			g.drawText( "Pitch", padding + 30, y_cols, 40, 20, ::juce::Justification::centredLeft );
			g.drawText( "Period", padding + 75, y_cols, 55, 20, ::juce::Justification::centredLeft );
			g.drawText( "Offset", padding + 135, y_cols, 65, 20, ::juce::Justification::centredLeft );

			// Divider
			const int list_top = y_cols + 22;
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
				const auto swatch_bounds = swatchRect( i, list_top );
				const auto colour        = colourFor( i );
				g.setColour( colour );
				g.fillRoundedRectangle( swatch_bounds.toFloat(), 3.0f );
				g.setColour( colour.brighter( 0.3f ) );
				g.drawRoundedRectangle( swatch_bounds.toFloat(), 3.0f, 1.0f );

				g.setColour( ::juce::Colours::white );
				g.drawText( ::juce::String( note.pitch ),             padding + 30,  y, 40, row_h, ::juce::Justification::centredLeft );
				g.drawText( ::juce::String( note.period, 2 ) + " b",  padding + 75,  y, 55, row_h, ::juce::Justification::centredLeft );
				g.drawText( ::juce::String( note.offset, 2 ) + " b",  padding + 135, y, 65, row_h, ::juce::Justification::centredLeft );

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
		static constexpr int swatch_w = 18;
		static constexpr int swatch_h = 14;
		static constexpr int padding  = 8;
		static constexpr int header_h = 30;
		static constexpr int row_h    = 28;

		std::vector<PeriodicNote>   m_notes;
		std::vector<::juce::Colour> m_colours;

		int listTop() const {
			return header_h + 4 + 22;
		}

		::juce::Rectangle<int> swatchRect( int i, int list_top ) const {
			const int y = list_top + i * row_h + ( row_h - swatch_h ) / 2;
			return { padding, y, swatch_w, swatch_h };
		}

		::juce::Rectangle<int> removeRect( int i, int list_top ) const {
			const int y = list_top + i * row_h + ( row_h - 16 ) / 2;
			return { getWidth() - padding - 16, y, 16, 16 };
		}

		::juce::Colour colourFor( int i ) const {
			if ( i < static_cast<int>( m_colours.size() ) )
				return m_colours[ i ];
			return ::juce::Colours::grey;
		}
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_NOTE_LIST_PANEL_HPP
