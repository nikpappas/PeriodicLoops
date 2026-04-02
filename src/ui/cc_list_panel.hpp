#ifndef PLOP_SRC_UI_CC_LIST_PANEL_HPP
#define PLOP_SRC_UI_CC_LIST_PANEL_HPP

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "music/midi.hpp"
#include "ui/colours.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	namespace {
		inline void drawWaveThumbnail( ::juce::Graphics &g, ::juce::Rectangle<int> bounds, WaveShape shape, bool selected ) {
			g.setColour( selected ? colours::accentOrange.withAlpha( 0.85f ) : colours::inputBg );
			g.fillRoundedRectangle( bounds.toFloat(), 2.0f );

			const float bx     = static_cast<float>( bounds.getX() );
			const float by     = static_cast<float>( bounds.getY() );
			const float bh     = static_cast<float>( bounds.getHeight() );
			const float margin = 2.5f;
			const float waveH  = bh - 2.0f * margin;
			const int   steps  = bounds.getWidth();

			::juce::Path wave;
			for ( int px = 0; px < steps; ++px ) {
				const float t   = static_cast<float>( px ) / static_cast<float>( steps );
				const float val = evalWaveShape( shape, t );
				const float py  = by + margin + waveH * ( 1.0f - val );
				if ( px == 0 )
					wave.startNewSubPath( bx + static_cast<float>( px ), py );
				else
					wave.lineTo( bx + static_cast<float>( px ), py );
			}
			g.setColour( selected ? ::juce::Colours::white : colours::offWhite.withAlpha( 0.6f ) );
			g.strokePath( wave, ::juce::PathStrokeType( 1.0f ) );
		}

		inline ::juce::String midiCCToName( int cc ) {
			switch ( cc ) {
				case 0:
					return "Bank Sel";
				case 1:
					return "Mod";
				case 2:
					return "Breath";
				case 4:
					return "Foot";
				case 5:
					return "Port.T";
				case 6:
					return "Data MSB";
				case 7:
					return "Volume";
				case 8:
					return "Balance";
				case 10:
					return "Pan";
				case 11:
					return "Expr";
				case 12:
					return "FX Ctl 1";
				case 13:
					return "FX Ctl 2";
				case 32:
					return "Bank LSB";
				case 38:
					return "Data LSB";
				case 64:
					return "Sustain";
				case 65:
					return "Port.Sw";
				case 66:
					return "Sostenuto";
				case 67:
					return "Soft Ped";
				case 68:
					return "Legato";
				case 71:
					return "Resonance";
				case 72:
					return "Rel.Time";
				case 73:
					return "Atk.Time";
				case 74:
					return "Brightness";
				case 84:
					return "Port.Ctl";
				case 91:
					return "Reverb";
				case 92:
					return "Tremolo";
				case 93:
					return "Chorus";
				case 94:
					return "Detune";
				case 95:
					return "Phaser";
				case 96:
					return "Data +1";
				case 97:
					return "Data -1";
				case 98:
					return "NRPN LSB";
				case 99:
					return "NRPN MSB";
				case 100:
					return "RPN LSB";
				case 101:
					return "RPN MSB";
				case 120:
					return "AllSndOff";
				case 121:
					return "Rst.Ctls";
				case 123:
					return "AllNoteOff";
				default:
					return "CC " + ::juce::String( cc );
			}
		}
	} // namespace

	class CcListPanel : public ::juce::Component {
	 public:
		struct Callbacks {
			std::function<void( int )>                     onRemoveCc;
			std::function<void()>                          onAddCc;
			std::function<void( int, ::plop::PeriodicCC )> onCcChanged;
		};

		using OnToggle = std::function<void()>;

		explicit CcListPanel( Callbacks cbs, OnToggle onToggle = nullptr ) :
				  mCbs( std::move( cbs ) ), mOnToggle( std::move( onToggle ) ) {
			mViewport.setScrollBarsShown( true, false );
			mViewport.setViewedComponent( &mRows, false );
			addAndMakeVisible( mViewport );

			mRows.onRemoveCc = [ this ]( int i ) {
				if ( mCbs.onRemoveCc )
					mCbs.onRemoveCc( i );
			};
			mRows.onCcChanged = [ this ]( int i, ::plop::PeriodicCC cc ) {
				if ( mCbs.onCcChanged )
					mCbs.onCcChanged( i, cc );
			};
		}

		int getCollapsedHeight() const {
			return HEADER_H;
		}

		void setCCs( std::vector<::plop::PeriodicCC> ccs ) {
			mRows.setCCs( std::move( ccs ) );
			syncRowsSize();
		}

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( colours::panelBg );

			g.setColour( colours::darkestGrey );
			g.setFont( ::juce::Font( FONT_LG, ::juce::Font::bold ) );
			g.drawText( "CC Events", PAD_MD + 14, 0, getWidth() - PAD_MD - 14, HEADER_H, ::juce::Justification::centredLeft );

			const auto addB = addButtonRect();
			g.setColour( colours::addBg );
			g.fillRoundedRectangle( addB.toFloat(), BTN_CORNER_RADIUS );
			g.setColour( colours::addAccent );
			g.setFont( FONT_LG );
			g.drawText( "+ Add CC", addB, ::juce::Justification::centred );

			g.setColour( colours::defText );
			g.setFont( FONT_SM );
			const int y_cols = HEADER_H + PAD_SM;
			g.drawText( "CC", PAD_MD, y_cols, 55, 20, ::juce::Justification::centredLeft );
			g.drawText( "~", PAD_MD + 68, y_cols, 40, 20, ::juce::Justification::centredLeft );
			g.drawText( "Period", PAD_MD + 111, y_cols, 50, 20, ::juce::Justification::centredLeft );
			g.drawText( "Offset", PAD_MD + 161, y_cols, 50, 20, ::juce::Justification::centredLeft );
			g.drawText( "Ch", PAD_MD + 205, y_cols, 22, 20, ::juce::Justification::centredLeft );

			g.setColour( colours::borderLine );
			g.drawHorizontalLine( TOTAL_HEADER_H - 2, 0.0f, static_cast<float>( getWidth() ) );
		}

		void mouseDown( const ::juce::MouseEvent &e ) override {
			if ( e.getPosition().y < HEADER_H ) {
				if ( mOnToggle )
					mOnToggle();
				repaint();
				return;
			}
			if ( mCbs.onAddCc && addButtonRect().contains( e.getPosition() ) )
				mCbs.onAddCc();
		}

		void resized() override {
			mViewport.setVisible( true );
			mViewport.setBounds( 0, TOTAL_HEADER_H, getWidth(), getHeight() - TOTAL_HEADER_H - ADD_BTN_H );
			syncRowsSize();
		}

	 private:
		static constexpr int HEADER_H       = 30;
		static constexpr int TOTAL_HEADER_H = HEADER_H + PAD_SM + 22; // 56
		static constexpr int ADD_BTN_H      = 36;

		const Callbacks mCbs;
		const OnToggle  mOnToggle;

		::juce::Rectangle<int> addButtonRect() const {
			return { PAD_MD, getHeight() - ADD_BTN_H + 7, getWidth() - 2 * PAD_MD, 22 };
		}

		void syncRowsSize() {
			if ( mViewport.getWidth() > 0 )
				mRows.setSize( mViewport.getWidth(), mRows.getContentHeight() );
		}

		// ---- scrollable rows -----------------------------------------------
		class RowsComponent : public ::juce::Component {
		 public:
			std::function<void( int )>                     onRemoveCc;
			std::function<void( int, ::plop::PeriodicCC )> onCcChanged;

			RowsComponent() {
				mEditor.setJustification( ::juce::Justification::centred );
				mEditor.setColour( ::juce::TextEditor::backgroundColourId, colours::inputBg );
				mEditor.setColour( ::juce::TextEditor::textColourId, ::juce::Colours::white );
				mEditor.setColour( ::juce::TextEditor::outlineColourId, colours::accentBlue );
				mEditor.onReturnKey = [ this ] { commitEdit(); };
				mEditor.onEscapeKey = [ this ] { cancelEdit(); };
				mEditor.onFocusLost = [ this ] { commitEdit(); };
				addChildComponent( mEditor );
			}

			void setCCs( const std::vector<::plop::PeriodicCC> &ccs ) {
				mCcs = ccs;
			}

			int getContentHeight() const {
				return static_cast<int>( mCcs.size() ) * ROW_H;
			}

			void paint( ::juce::Graphics &g ) override {
				g.fillAll( colours::panelBg );
				g.setFont( FONT_LG );
				for ( int i = 0; i < static_cast<int>( mCcs.size() ); ++i ) {
					const auto &cc = mCcs[ i ];
					const int   y  = i * ROW_H;
					if ( i % 2 == 0 ) {
						g.setColour( colours::rowAlt );
						g.fillRect( 0, y, getWidth(), ROW_H );
					}
					g.setColour( ::juce::Colours::white );
					drawCell( g, midiCCToName( cc.number ), numberRect( i ) );
					drawWaveThumbnail( g, shapeRect( i, 0 ), ::plop::WaveShape::Sin, cc.shape == ::plop::WaveShape::Sin );
					drawWaveThumbnail( g, shapeRect( i, 1 ), ::plop::WaveShape::Tri, cc.shape == ::plop::WaveShape::Tri );
					drawWaveThumbnail( g, shapeRect( i, 2 ), ::plop::WaveShape::Saw, cc.shape == ::plop::WaveShape::Saw );
					drawCell( g, ::juce::String( cc.period, 2 ) + " b", periodRect( i ) );
					drawCell( g, ::juce::String( cc.offset, 2 ) + " b", offsetRect( i ) );
					drawCell( g, ::juce::String( cc.channel ), channelRect( i ) );

					const auto sb = soloRect( i );
					g.setColour( colours::accentOrange );
					g.fillRoundedRectangle( sb.toFloat(), BTN_CORNER_RADIUS );
					g.setColour( mCcs[ i ].solo ? ::juce::Colours::white : colours::offWhite );
					g.setFont( FONT_SM );
					g.drawText( "S", sb, ::juce::Justification::centred );

					const auto rb = removeRect( i );
					g.setColour( colours::removeBg );
					g.fillRoundedRectangle( rb.toFloat(), BTN_CORNER_RADIUS );
					g.setColour( colours::removeAccent );
					g.setFont( FONT_SM );
					g.drawText( "x", rb, ::juce::Justification::centred );
				}
			}

			void mouseDown( const ::juce::MouseEvent &e ) override {
				const auto pos = e.getPosition();
				for ( int i = 0; i < static_cast<int>( mCcs.size() ); ++i ) {
					if ( onRemoveCc && removeRect( i ).contains( pos ) ) {
						onRemoveCc( i );
						return;
					}

					if ( soloRect( i ).contains( pos ) ) {
						::plop::PeriodicCC updated = mCcs[ i ];
						updated.solo               = !updated.solo;
						onCcChanged( i, updated );
						if ( !e.mods.isCtrlDown() ) {
							for ( int j = 0; j < static_cast<int>( mCcs.size() ); ++j ) {
								if ( j == i || !mCcs[ j ].solo )
									continue;
								::plop::PeriodicCC other = mCcs[ j ];
								other.solo               = false;
								onCcChanged( j, other );
							}
						}
						return;
					}
					for ( int s = 0; s < 3; ++s ) {
						if ( shapeRect( i, s ).contains( pos ) ) {
							::plop::PeriodicCC updated = mCcs[ i ];
							updated.shape              = static_cast<::plop::WaveShape>( s );
							mCcs[ i ]                  = updated;
							if ( onCcChanged )
								onCcChanged( i, updated );
							repaint();
							return;
						}
					}

					Field f = Field::None;
					if ( numberRect( i ).contains( pos ) )
						f = Field::Number;
					else if ( periodRect( i ).contains( pos ) )
						f = Field::Period;
					else if ( offsetRect( i ).contains( pos ) )
						f = Field::Offset;
					else if ( channelRect( i ).contains( pos ) )
						f = Field::Channel;

					if ( f != Field::None ) {
						mDragIndex   = i;
						mDragField   = f;
						mDragStartY  = pos.y;
						mDragStartCc = mCcs[ i ];
						setMouseCursor( ::juce::MouseCursor::UpDownResizeCursor );
						return;
					}
				}
			}

			void mouseDrag( const ::juce::MouseEvent &e ) override {
				if ( mDragIndex < 0 || mDragField == Field::None )
					return;
				const int dy = mDragStartY - e.getPosition().y;

				::plop::PeriodicCC updated = mDragStartCc;
				if ( mDragField == Field::Number )
					updated.number = ::juce::jlimit( 0, 127, mDragStartCc.number + dy / 3 );
				else if ( mDragField == Field::Period )
					updated.period = ::juce::jmax( 0.01f, mDragStartCc.period + dy * 0.05f );
				else if ( mDragField == Field::Offset )
					updated.offset = ::juce::jlimit( 0.0f, updated.period, mDragStartCc.offset + dy * 0.05f );
				else
					updated.channel = ::juce::jlimit( 0, 15, mDragStartCc.channel + dy / 8 );

				mCcs[ mDragIndex ] = updated;
				onCcChanged( mDragIndex, updated );
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
					if ( numberRect( i ).contains( e.getPosition() ) ) {
						startEdit( i, Field::Number );
						return;
					}
					if ( periodRect( i ).contains( e.getPosition() ) ) {
						startEdit( i, Field::Period );
						return;
					}
					if ( offsetRect( i ).contains( e.getPosition() ) ) {
						startEdit( i, Field::Offset );
						return;
					}
					if ( channelRect( i ).contains( e.getPosition() ) ) {
						startEdit( i, Field::Channel );
						return;
					}
				}
			}

		 private:
			enum class Field { None, Number, Period, Offset, Channel };
			static constexpr int ROW_H = 28;

			std::vector<::plop::PeriodicCC> mCcs;
			::juce::TextEditor              mEditor;

			int   mEditingIndex = -1;
			Field mEditingField = Field::None;

			int                mDragIndex   = -1;
			Field              mDragField   = Field::None;
			int                mDragStartY  = 0;
			::plop::PeriodicCC mDragStartCc = {};

			::juce::Rectangle<int> numberRect( int i ) const {
				return { PAD_MD, i * ROW_H, 55, ROW_H };
			}
			/// s = 0 (Sin), 1 (Tri), 2 (Saw)
			::juce::Rectangle<int> shapeRect( int i, int s ) const {
				return { PAD_MD + 57 + s * 18, i * ROW_H + ( ROW_H - 16 ) / 2, 16, 16 };
			}
			::juce::Rectangle<int> periodRect( int i ) const {
				return { PAD_MD + 111, i * ROW_H, 50, ROW_H };
			}
			::juce::Rectangle<int> offsetRect( int i ) const {
				return { PAD_MD + 164, i * ROW_H, 50, ROW_H };
			}

			::juce::Rectangle<int> channelRect( int i ) const {
				return { PAD_MD + 224, i * ROW_H, 22, ROW_H };
			}
			::juce::Rectangle<int> removeRect( int i ) const {
				return { getWidth() - PAD_MD - 16, i * ROW_H + ( ROW_H - 16 ) / 2, 16, 16 };
			}
			::juce::Rectangle<int> soloRect( int i ) const {
				return { getWidth() - PAD_MD - 46, i * ROW_H + ( ROW_H - 16 ) / 2, 16, 16 };
			}

			void drawCell( ::juce::Graphics &g, const ::juce::String &text, ::juce::Rectangle<int> bounds ) const {
				g.setColour( colours::darkestGrey );
				g.drawText( text, bounds, ::juce::Justification::centredLeft );
			}

			void startEdit( int i, Field field ) {
				mEditingIndex = i;
				mEditingField = field;
				::juce::Rectangle<int> bounds;
				::juce::String         text;
				if ( field == Field::Number ) {
					bounds = numberRect( i );
					text   = ::juce::String( mCcs[ i ].number );
					mEditor.setInputRestrictions( 3, "0123456789" );
				} else if ( field == Field::Period ) {
					bounds = periodRect( i );
					text   = ::juce::String( mCcs[ i ].period );
					mEditor.setInputRestrictions( 8, "0123456789." );
				} else if ( field == Field::Offset ) {
					bounds = offsetRect( i );
					text   = ::juce::String( mCcs[ i ].offset );
					mEditor.setInputRestrictions( 8, "0123456789." );
				} else {
					bounds = channelRect( i );
					text   = ::juce::String( mCcs[ i ].channel );
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
				::plop::PeriodicCC updated = mCcs[ mEditingIndex ];
				if ( mEditingField == Field::Number )
					updated.number = ::juce::jlimit( 0, 127, mEditor.getText().getIntValue() );
				else if ( mEditingField == Field::Period )
					updated.period = ::juce::jmax( 0.01f, mEditor.getText().getFloatValue() );
				else if ( mEditingField == Field::Offset )
					updated.offset = ::juce::jlimit( 0.0f, updated.period, mEditor.getText().getFloatValue() );
				else
					updated.channel = ::juce::jlimit( 0, 15, mEditor.getText().getIntValue() );
				if ( onCcChanged )
					onCcChanged( mEditingIndex, updated );
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
