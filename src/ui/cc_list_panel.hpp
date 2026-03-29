#ifndef PLOP_SRC_UI_CC_LIST_PANEL_HPP
#define PLOP_SRC_UI_CC_LIST_PANEL_HPP

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/midi.hpp"

namespace plop::ui {

	namespace {
		inline ::juce::String midiCCToName( int cc ) {
			switch ( cc ) {
				case 0:   return "Bank Sel";
				case 1:   return "Mod";
				case 2:   return "Breath";
				case 4:   return "Foot";
				case 5:   return "Port.T";
				case 6:   return "Data MSB";
				case 7:   return "Volume";
				case 8:   return "Balance";
				case 10:  return "Pan";
				case 11:  return "Expr";
				case 12:  return "FX Ctl 1";
				case 13:  return "FX Ctl 2";
				case 32:  return "Bank LSB";
				case 38:  return "Data LSB";
				case 64:  return "Sustain";
				case 65:  return "Port.Sw";
				case 66:  return "Sostenuto";
				case 67:  return "Soft Ped";
				case 68:  return "Legato";
				case 71:  return "Resonance";
				case 72:  return "Rel.Time";
				case 73:  return "Atk.Time";
				case 74:  return "Brightness";
				case 84:  return "Port.Ctl";
				case 91:  return "Reverb";
				case 92:  return "Tremolo";
				case 93:  return "Chorus";
				case 94:  return "Detune";
				case 95:  return "Phaser";
				case 96:  return "Data +1";
				case 97:  return "Data -1";
				case 98:  return "NRPN LSB";
				case 99:  return "NRPN MSB";
				case 100: return "RPN LSB";
				case 101: return "RPN MSB";
				case 120: return "AllSndOff";
				case 121: return "Rst.Ctls";
				case 123: return "AllNoteOff";
				default:  return "CC " + ::juce::String( cc );
			}
		}
	} // namespace

	class CcListPanel : public ::juce::Component {
	 public:
		struct Callbacks {
			std::function<void( int )>             onRemoveCc;
			std::function<void()>                  onAddCc;
			std::function<void( int, PeriodicCC )> onCcChanged;
		};

		using OnToggle = std::function<void()>;

		explicit CcListPanel( Callbacks cbs, OnToggle onToggle = nullptr )
		    : mCbs( std::move( cbs ) ), mOnToggle( std::move( onToggle ) ) {
			mViewport.setScrollBarsShown( true, false );
			mViewport.setViewedComponent( &mRows, false );
			addAndMakeVisible( mViewport );

			mRows.onRemoveCc  = [ this ]( int i ) { if ( mCbs.onRemoveCc ) mCbs.onRemoveCc( i ); };
			mRows.onAddCc     = [ this ]() { if ( mCbs.onAddCc ) mCbs.onAddCc(); };
			mRows.onCcChanged = [ this ]( int i, PeriodicCC cc ) { if ( mCbs.onCcChanged ) mCbs.onCcChanged( i, cc ); };
		}

		bool isCollapsed() const { return mCollapsed; }

		int getCollapsedHeight() const { return k_header_h; }

		void setCCs( std::vector<PeriodicCC> ccs ) {
			mRows.setCCs( std::move( ccs ) );
			syncRowsSize();
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( ::juce::Colour( 0xff12121f ) );

			g.setColour( ::juce::Colour( 0xff445555 ) );
			g.fillRect( 0, 0, getWidth(), k_header_h );

			// Collapse/expand triangle
			const float triX = static_cast<float>( k_padding );
			const float triY = static_cast<float>( k_header_h ) / 2.0f;
			::juce::Path tri;
			if ( mCollapsed ) {
				tri.addTriangle( triX, triY - 4.0f, triX, triY + 4.0f, triX + 6.0f, triY );
			} else {
				tri.addTriangle( triX, triY - 3.0f, triX + 8.0f, triY - 3.0f, triX + 4.0f, triY + 4.0f );
			}
			g.setColour( ::juce::Colours::white.withAlpha( 0.7f ) );
			g.fillPath( tri );

			g.setColour( ::juce::Colours::white );
			g.setFont( ::juce::Font( 13.0f, ::juce::Font::bold ) );
			g.drawText( "CC Events", k_padding + 14, 0, getWidth() - k_padding - 14, k_header_h, ::juce::Justification::centredLeft );

			if ( mCollapsed )
				return;

			g.setColour( ::juce::Colour( 0xff888899 ) );
			g.setFont( 11.0f );
			const int y_cols = k_header_h + 4;
			g.drawText( "CC",     k_padding,        y_cols, 55, 20, ::juce::Justification::centredLeft );
			g.drawText( "Period", k_padding + 58,   y_cols, 50, 20, ::juce::Justification::centredLeft );
			g.drawText( "Offset", k_padding + 111,  y_cols, 50, 20, ::juce::Justification::centredLeft );
			g.drawText( "Ch",     k_padding + 164,  y_cols, 22, 20, ::juce::Justification::centredLeft );

			g.setColour( ::juce::Colour( 0xff333344 ) );
			g.drawHorizontalLine( k_total_header_h - 2, 0.0f, static_cast<float>( getWidth() ) );
		}

		void mouseDown( const ::juce::MouseEvent &e ) override {
			if ( e.getPosition().y < k_header_h ) {
				mCollapsed = !mCollapsed;
				mViewport.setVisible( !mCollapsed );
				if ( mOnToggle )
					mOnToggle();
				repaint();
			}
		}

		void resized() override {
			if ( mCollapsed ) {
				mViewport.setVisible( false );
				return;
			}
			mViewport.setVisible( true );
			mViewport.setBounds( 0, k_total_header_h, getWidth(), getHeight() - k_total_header_h );
			syncRowsSize();
		}

	 private:
		static constexpr int k_padding        = 8;
		static constexpr int k_header_h       = 30;
		static constexpr int k_total_header_h = k_header_h + 4 + 22; // 56

		const Callbacks mCbs;
		const OnToggle  mOnToggle;
		bool            mCollapsed = true;

		void syncRowsSize() {
			if ( mViewport.getWidth() > 0 )
				mRows.setSize( mViewport.getWidth(), mRows.getContentHeight() );
		}

		// ---- scrollable rows -----------------------------------------------
		class RowsComponent : public ::juce::Component {
		 public:
			std::function<void( int )>             onRemoveCc;
			std::function<void()>                  onAddCc;
			std::function<void( int, PeriodicCC )> onCcChanged;

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

			void setCCs( std::vector<PeriodicCC> ccs ) { mCcs = std::move( ccs ); }

			int getContentHeight() const {
				return static_cast<int>( mCcs.size() ) * row_h + 6 + 22 + 8;
			}

			void paint( ::juce::Graphics &g ) override {
				g.fillAll( ::juce::Colour( 0xff12121f ) );
				g.setFont( 13.0f );
				for ( int i = 0; i < static_cast<int>( mCcs.size() ); ++i ) {
					const auto &cc = mCcs[ i ];
					const int   y  = i * row_h;
					if ( i % 2 == 0 ) {
						g.setColour( ::juce::Colour( 0xff1e1e30 ) );
						g.fillRect( 0, y, getWidth(), row_h );
					}
					g.setColour( ::juce::Colours::white );
					drawCell( g, midiCCToName( cc.number ),               numberRect( i ),  i == mEditingIndex && mEditingField == Field::Number );
					drawCell( g, ::juce::String( cc.period, 2 ) + " b",  periodRect( i ),  i == mEditingIndex && mEditingField == Field::Period );
					drawCell( g, ::juce::String( cc.offset, 2 ) + " b",  offsetRect( i ),  i == mEditingIndex && mEditingField == Field::Offset );
					drawCell( g, ::juce::String( cc.channel ),            channelRect( i ), i == mEditingIndex && mEditingField == Field::Channel );

					const auto rb = removeRect( i );
					g.setColour( ::juce::Colour( 0xff553333 ) );
					g.fillRoundedRectangle( rb.toFloat(), 3.0f );
					g.setColour( ::juce::Colour( 0xffff6666 ) );
					g.setFont( 11.0f );
					g.drawText( "x", rb, ::juce::Justification::centred );
					g.setFont( 13.0f );
				}
				const int  addY = static_cast<int>( mCcs.size() ) * row_h + 6;
				const auto addB = ::juce::Rectangle<int>{ k_padding, addY, getWidth() - 2 * k_padding, 22 };
				g.setColour( ::juce::Colour( 0xff223322 ) );
				g.fillRoundedRectangle( addB.toFloat(), 4.0f );
				g.setColour( ::juce::Colour( 0xff66cc66 ) );
				g.setFont( 13.0f );
				g.drawText( "+ Add CC", addB, ::juce::Justification::centred );
			}

			void mouseDown( const ::juce::MouseEvent &e ) override {
				const auto pos = e.getPosition();
				for ( int i = 0; i < static_cast<int>( mCcs.size() ); ++i ) {
					if ( onRemoveCc && removeRect( i ).contains( pos ) ) { onRemoveCc( i ); return; }

					Field f = Field::None;
					if ( numberRect( i ).contains( pos ) )       f = Field::Number;
					else if ( periodRect( i ).contains( pos ) )  f = Field::Period;
					else if ( offsetRect( i ).contains( pos ) )  f = Field::Offset;
					else if ( channelRect( i ).contains( pos ) ) f = Field::Channel;

					if ( f != Field::None ) {
						mDragIndex    = i;
						mDragField    = f;
						mDragStartY  = pos.y;
						mDragStartCc = mCcs[ i ];
						setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
						return;
					}
				}
				const int  addY = static_cast<int>( mCcs.size() ) * row_h + 6;
				const auto addB = ::juce::Rectangle<int>{ k_padding, addY, getWidth() - 2 * k_padding, 22 };
				if ( onAddCc && addB.contains( pos ) ) onAddCc();
			}

			void mouseDrag( const ::juce::MouseEvent &e ) override {
				if ( mDragIndex < 0 || mDragField == Field::None ) return;
				const int dy = mDragStartY - e.getPosition().y;

				PeriodicCC updated = mDragStartCc;
				if ( mDragField == Field::Number )
					updated.number = ::juce::jlimit( 0, 127, mDragStartCc.number + dy / 3 );
				else if ( mDragField == Field::Period )
					updated.period = ::juce::jmax( 0.01f, mDragStartCc.period + dy * 0.05f );
				else if ( mDragField == Field::Offset )
					updated.offset = ::juce::jlimit( 0.0f, updated.period, mDragStartCc.offset + dy * 0.05f );
				else
					updated.channel = ::juce::jlimit( 0, 15, mDragStartCc.channel + dy / 8 );

				mCcs[ mDragIndex ] = updated;
				if ( onCcChanged ) onCcChanged( mDragIndex, updated );
				repaint();
			}

			void mouseUp( const ::juce::MouseEvent & ) override {
				if ( mDragField != Field::None )
					setMouseCursor( ::juce::MouseCursor::NormalCursor );
				mDragIndex = -1;
				mDragField = Field::None;
			}

			void mouseDoubleClick( const ::juce::MouseEvent &e ) override {
				for ( int i = 0; i < static_cast<int>( mCcs.size() ); ++i ) {
					if ( numberRect( i ).contains( e.getPosition() ) )  { startEdit( i, Field::Number );  return; }
					if ( periodRect( i ).contains( e.getPosition() ) )  { startEdit( i, Field::Period );  return; }
					if ( offsetRect( i ).contains( e.getPosition() ) )  { startEdit( i, Field::Offset );  return; }
					if ( channelRect( i ).contains( e.getPosition() ) ) { startEdit( i, Field::Channel ); return; }
				}
			}

		 private:
			enum class Field { None, Number, Period, Offset, Channel };
			static constexpr int k_padding = 8;
			static constexpr int row_h     = 28;

			std::vector<PeriodicCC> mCcs;
			::juce::TextEditor      mEditor;
			int                     mEditingIndex = -1;
			Field                   mEditingField = Field::None;

			int        mDragIndex    = -1;
			Field      mDragField    = Field::None;
			int        mDragStartY  = 0;
			PeriodicCC mDragStartCc = {};

			::juce::Rectangle<int> numberRect( int i ) const  { return { k_padding,        i * row_h, 55, row_h }; }
			::juce::Rectangle<int> periodRect( int i ) const  { return { k_padding + 58,   i * row_h, 50, row_h }; }
			::juce::Rectangle<int> offsetRect( int i ) const  { return { k_padding + 111,  i * row_h, 50, row_h }; }
			::juce::Rectangle<int> channelRect( int i ) const { return { k_padding + 164,  i * row_h, 22, row_h }; }
			::juce::Rectangle<int> removeRect( int i ) const {
				return { getWidth() - k_padding - 16, i * row_h + ( row_h - 16 ) / 2, 16, 16 };
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
				if ( field == Field::Number ) {
					bounds = numberRect( i );  text = ::juce::String( mCcs[ i ].number );
					mEditor.setInputRestrictions( 3, "0123456789" );
				} else if ( field == Field::Period ) {
					bounds = periodRect( i );  text = ::juce::String( mCcs[ i ].period );
					mEditor.setInputRestrictions( 8, "0123456789." );
				} else if ( field == Field::Offset ) {
					bounds = offsetRect( i );  text = ::juce::String( mCcs[ i ].offset );
					mEditor.setInputRestrictions( 8, "0123456789." );
				} else {
					bounds = channelRect( i ); text = ::juce::String( mCcs[ i ].channel );
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
				PeriodicCC updated = mCcs[ mEditingIndex ];
				if ( mEditingField == Field::Number )
					updated.number = ::juce::jlimit( 0, 127, mEditor.getText().getIntValue() );
				else if ( mEditingField == Field::Period )
					updated.period = ::juce::jmax( 0.01f, mEditor.getText().getFloatValue() );
				else if ( mEditingField == Field::Offset )
					updated.offset = ::juce::jlimit( 0.0f, updated.period, mEditor.getText().getFloatValue() );
				else
					updated.channel = ::juce::jlimit( 0, 15, mEditor.getText().getIntValue() );
				if ( onCcChanged ) onCcChanged( mEditingIndex, updated );
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

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( CcListPanel )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_CC_LIST_PANEL_HPP
