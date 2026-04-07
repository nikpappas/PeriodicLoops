#ifndef PLOP_SRC_UI_CIRCLE_GRID_HPP
#define PLOP_SRC_UI_CIRCLE_GRID_HPP

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ui/colours.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	/// A single circular selector button.
	/// Agnostic of domain — accepts only title, isSelected, isEmpty, and a select callback.
	class CircleSelectorButton : public ::juce::Component {
	 public:
		CircleSelectorButton() = default;

		void set( const ::juce::String &title, bool isSelected, bool isEmpty, std::function<void()> onSelect ) {
			mTitle      = title;
			mIsSelected = isSelected;
			mIsEmpty    = isEmpty;
			mOnSelect   = std::move( onSelect );
			repaint();
		}

		void paint( ::juce::Graphics &g ) override {
			constexpr float PAD = 2.0f;
			const float diam = std::min( static_cast<float>( getWidth() ), static_cast<float>( getHeight() ) ) - 2.0f * PAD;
			const float cx = static_cast<float>( getWidth() ) / 2.0f;
			const float cy = static_cast<float>( getHeight() ) / 2.0f;
			const float r  = diam / 2.0f;

			if ( mIsEmpty ) {
				::juce::Path ellipsePath;
				ellipsePath.addEllipse( cx - r, cy - r, diam, diam );
				::juce::Path dashed;
				const float  dashLengths[] = { 3.0f, 4.0f };
				::juce::PathStrokeType( 1.0f ).createDashedStroke( dashed, ellipsePath, dashLengths, 2 );
				g.setColour( colours::subtleGrey.withAlpha( 0.6f ) );
				g.fillPath( dashed );
				return;
			}

			if ( mIsSelected ) {
				g.setColour( colours::darkestGrey );
				g.fillEllipse( cx - r, cy - r, diam, diam );
				// Outer ring
				g.setColour( ::juce::Colours::white.withAlpha( 0.45f ) );
				g.drawEllipse( cx - r - 2.5f, cy - r - 2.5f, diam + 5.0f, diam + 5.0f, 1.5f );
			} else {
				g.setColour( colours::lightestGrey );
				g.fillEllipse( cx - r, cy - r, diam, diam );
				g.setColour( colours::darkestGrey );
				g.drawEllipse( cx - r, cy - r, diam, diam, 1.0f );
			}

			g.setColour( mIsSelected ? ::juce::Colours::white : colours::offWhite );
			g.setFont( FONT_SM );
			g.drawText( mTitle, getLocalBounds(), ::juce::Justification::centred );
		}

		void mouseDown( const ::juce::MouseEvent & ) override {
			if ( !mIsEmpty && mOnSelect )
				mOnSelect();
		}

	 private:
		::juce::String        mTitle;
		bool                  mIsSelected = false;
		bool                  mIsEmpty    = true;
		std::function<void()> mOnSelect;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( CircleSelectorButton )
	};

	/// A 4 × 3 grid of CircleSelectorButtons. Row-major indexing (0 = top-left, 11 = bottom-right).
	class CircleGrid : public ::juce::Component {
	 public:
		static constexpr int COLS = 4;
		static constexpr int ROWS = 3;
		static constexpr int N    = COLS * ROWS;

		CircleGrid() {
			for ( auto &btn : mButtons )
				addAndMakeVisible( btn );
		}

		/// Configure one slot. index is row-major [0, N).
		void setButton( int index, const ::juce::String &title, bool isSelected, bool isEmpty, std::function<void()> onSelect ) {
			if ( index >= 0 && index < N )
				mButtons[ static_cast<size_t>( index ) ].set( title, isSelected, isEmpty, std::move( onSelect ) );
		}

		void resized() override {
			const int cellW = getWidth() / COLS;
			const int cellH = getHeight() / ROWS;
			for ( int i = 0; i < N; ++i )
				mButtons[ static_cast<size_t>( i ) ].setBounds( ( i % COLS ) * cellW, ( i / COLS ) * cellH, cellW, cellH );
		}

	 private:
		std::array<CircleSelectorButton, N> mButtons;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( CircleGrid )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_CIRCLE_GRID_HPP
