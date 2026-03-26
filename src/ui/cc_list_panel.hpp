#ifndef PLOP_SRC_UI_CC_LIST_PANEL_HPP
#define PLOP_SRC_UI_CC_LIST_PANEL_HPP

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/midi.hpp"

namespace plop::ui {

	class CcListPanel : public ::juce::Component {
	 public:
		std::function<void( int index )>                                    onRemoveCc;
		std::function<void()>                                               onAddCc;
		/// Fired when any editable field changes. The full updated CC is provided.
		std::function<void( int index, PeriodicCC cc )>                     onCcChanged;

		CcListPanel() {
			m_editor.setJustification( ::juce::Justification::centred );
			m_editor.setColour( ::juce::TextEditor::backgroundColourId, ::juce::Colour( 0xff2a2a44 ) );
			m_editor.setColour( ::juce::TextEditor::textColourId, ::juce::Colours::white );
			m_editor.setColour( ::juce::TextEditor::outlineColourId, ::juce::Colour( 0xff4fc3f7 ) );
			m_editor.onReturnKey = [this] { commitEdit(); };
			m_editor.onEscapeKey = [this] { cancelEdit(); };
			m_editor.onFocusLost = [this] { commitEdit(); };
			addChildComponent( m_editor );
		}

		void setCCs( std::vector<PeriodicCC> ccs ) {
			m_ccs = std::move( ccs );
		}

		int getContentHeight() const {
			return listTop() + static_cast<int>( m_ccs.size() ) * row_h + 6 + 22 + 8;
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( ::juce::Colour( 0xff12121f ) );

			// Header
			g.setColour( ::juce::Colour( 0xff445555 ) );
			g.fillRect( 0, 0, getWidth(), header_h );
			g.setColour( ::juce::Colours::white );
			g.setFont( ::juce::Font( 13.0f, ::juce::Font::bold ) );
			g.drawText( "CC Events", padding, 0, getWidth() - padding, header_h, ::juce::Justification::centredLeft );

			// Column headers
			g.setColour( ::juce::Colour( 0xff888899 ) );
			g.setFont( 11.0f );
			const int y_cols = header_h + 4;
			g.drawText( "CC#",    padding,        y_cols, 32, 20, ::juce::Justification::centredLeft );
			g.drawText( "Period", padding + 35,   y_cols, 55, 20, ::juce::Justification::centredLeft );
			g.drawText( "Offset", padding + 95,   y_cols, 55, 20, ::juce::Justification::centredLeft );
			g.drawText( "Ch",     padding + 153,  y_cols, 22, 20, ::juce::Justification::centredLeft );

			// Divider
			const int list_top = listTop();
			g.setColour( ::juce::Colour( 0xff333344 ) );
			g.drawHorizontalLine( list_top - 2, 0.0f, static_cast<float>( getWidth() ) );

			// Rows
			g.setFont( 13.0f );
			for ( int i = 0; i < static_cast<int>( m_ccs.size() ); ++i ) {
				const auto &cc = m_ccs[ i ];
				const int   y  = list_top + i * row_h;

				if ( i % 2 == 0 ) {
					g.setColour( ::juce::Colour( 0xff1e1e30 ) );
					g.fillRect( 0, y, getWidth(), row_h );
				}

				g.setColour( ::juce::Colours::white );
				drawEditableCell( g, ::juce::String( cc.number ),            numberRect( i, list_top ),  i == m_editing_index && m_editing_field == Field::Number );
				drawEditableCell( g, ::juce::String( cc.period, 2 ) + " b",  periodRect( i, list_top ),  i == m_editing_index && m_editing_field == Field::Period );
				drawEditableCell( g, ::juce::String( cc.offset, 2 ) + " b",  offsetRect( i, list_top ),  i == m_editing_index && m_editing_field == Field::Offset );
				drawEditableCell( g, ::juce::String( cc.channel ),            channelRect( i, list_top ), i == m_editing_index && m_editing_field == Field::Channel );

				// Remove button
				const auto rb = removeRect( i, list_top );
				g.setColour( ::juce::Colour( 0xff553333 ) );
				g.fillRoundedRectangle( rb.toFloat(), 3.0f );
				g.setColour( ::juce::Colour( 0xffff6666 ) );
				g.setFont( 11.0f );
				g.drawText( "x", rb, ::juce::Justification::centred );
			}

			// Add button
			const int  addY = list_top + static_cast<int>( m_ccs.size() ) * row_h + 6;
			const auto addB = ::juce::Rectangle<int>{ padding, addY, getWidth() - 2 * padding, 22 };
			g.setColour( ::juce::Colour( 0xff223322 ) );
			g.fillRoundedRectangle( addB.toFloat(), 4.0f );
			g.setColour( ::juce::Colour( 0xff66cc66 ) );
			g.setFont( 13.0f );
			g.drawText( "+ Add CC", addB, ::juce::Justification::centred );
		}

		void mouseDown( const ::juce::MouseEvent &e ) override {
			const int list_top = listTop();

			for ( int i = 0; i < static_cast<int>( m_ccs.size() ); ++i ) {
				if ( numberRect( i, list_top ).contains( e.getPosition() ) ) {
					startEdit( i, Field::Number, list_top );
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
				if ( channelRect( i, list_top ).contains( e.getPosition() ) ) {
					startEdit( i, Field::Channel, list_top );
					return;
				}
				if ( onRemoveCc && removeRect( i, list_top ).contains( e.getPosition() ) ) {
					onRemoveCc( i );
					return;
				}
			}

			const int  addY = list_top + static_cast<int>( m_ccs.size() ) * row_h + 6;
			const auto addB = ::juce::Rectangle<int>{ padding, addY, getWidth() - 2 * padding, 22 };
			if ( onAddCc && addB.contains( e.getPosition() ) )
				onAddCc();
		}

	 private:
		enum class Field { None, Number, Period, Offset, Channel };

		static constexpr int padding  = 8;
		static constexpr int header_h = 30;
		static constexpr int row_h    = 28;

		std::vector<PeriodicCC> m_ccs;
		::juce::TextEditor      m_editor;
		int                     m_editing_index = -1;
		Field                   m_editing_field = Field::None;

		int listTop() const { return header_h + 4 + 22; }

		::juce::Rectangle<int> numberRect( int i, int lt ) const {
			return { padding, lt + i * row_h, 32, row_h };
		}
		::juce::Rectangle<int> periodRect( int i, int lt ) const {
			return { padding + 35, lt + i * row_h, 55, row_h };
		}
		::juce::Rectangle<int> offsetRect( int i, int lt ) const {
			return { padding + 95, lt + i * row_h, 55, row_h };
		}
		::juce::Rectangle<int> channelRect( int i, int lt ) const {
			return { padding + 153, lt + i * row_h, 22, row_h };
		}
		::juce::Rectangle<int> removeRect( int i, int lt ) const {
			return { getWidth() - padding - 16, lt + i * row_h + ( row_h - 16 ) / 2, 16, 16 };
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

			if ( field == Field::Number ) {
				bounds = numberRect( i, list_top );
				text   = ::juce::String( m_ccs[ i ].number );
				m_editor.setInputRestrictions( 3, "0123456789" );
			} else if ( field == Field::Period ) {
				bounds = periodRect( i, list_top );
				text   = ::juce::String( m_ccs[ i ].period );
				m_editor.setInputRestrictions( 8, "0123456789." );
			} else if ( field == Field::Offset ) {
				bounds = offsetRect( i, list_top );
				text   = ::juce::String( m_ccs[ i ].offset );
				m_editor.setInputRestrictions( 8, "0123456789." );
			} else {
				bounds = channelRect( i, list_top );
				text   = ::juce::String( m_ccs[ i ].channel );
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

			PeriodicCC updated = m_ccs[ m_editing_index ];

			if ( m_editing_field == Field::Number ) {
				updated.number = ::juce::jlimit( 0, 127, m_editor.getText().getIntValue() );
			} else if ( m_editing_field == Field::Period ) {
				updated.period = ::juce::jmax( 0.01f, m_editor.getText().getFloatValue() );
			} else if ( m_editing_field == Field::Offset ) {
				updated.offset = ::juce::jmax( 0.0f, m_editor.getText().getFloatValue() );
			} else {
				updated.channel = ::juce::jlimit( 0, 15, m_editor.getText().getIntValue() );
			}

			if ( onCcChanged ) onCcChanged( m_editing_index, updated );
			cancelEdit();
		}

		void cancelEdit() {
			m_editor.setVisible( false );
			m_editing_index = -1;
			m_editing_field = Field::None;
		}
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_CC_LIST_PANEL_HPP
