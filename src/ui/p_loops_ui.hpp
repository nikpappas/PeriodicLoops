#ifndef PLOP_SRC_UI_P_LOOPS_UI_HPP
#define PLOP_SRC_UI_P_LOOPS_UI_HPP

#include <cmath>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "processor/periodic_loops.hpp"
#include "ui/cc_list_panel.hpp"
#include "ui/note_list_panel.hpp"
#include "ui/orbital_display.hpp"
#include "ui/time_display.hpp"

namespace plop::ui {

	class p_loops_ui
			  : public ::juce::AudioProcessorEditor
			  , private ::juce::Timer
			  , private ::juce::ChangeListener {
	 private:
		::plop::p_loops::p_loops &mPluginInstanceRef;
		TimeDisplay               mTimeDisplay;
		OrbitalDisplay            mOrbitalDisplay;
		NoteListPanel             mNoteListPanel;
		CcListPanel               mCcListPanel;
		int64_t                   mLastTime = 0;

		::juce::TextButton mBtnPro    { "Pro"    };
		::juce::TextButton mBtnMelody { "Melody" };
		::juce::TextButton mBtnDrums  { "Drums"  };
		NoteListPanel::Mode mMode = NoteListPanel::Mode::Melody;

		std::vector<::juce::Colour>                            mNoteColours;
		::juce::Component::SafePointer<::juce::ColourSelector> mActiveSelector;
		int                                                    mEditingIndex = -1;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( p_loops_ui )

	 public:
		explicit p_loops_ui( ::plop::p_loops::p_loops &owner );
		~p_loops_ui();

	 private:
		::juce::Colour nextPaletteColour() const {
			static const ::juce::Colour palette[] = {
				::juce::Colour( 0xff4fc3f7 ), ::juce::Colour( 0xffef5350 ), ::juce::Colour( 0xff66bb6a ),
				::juce::Colour( 0xffffa726 ), ::juce::Colour( 0xffab47bc ), ::juce::Colour( 0xff26c6da ),
				::juce::Colour( 0xffec407a ), ::juce::Colour( 0xff8d6e63 ),
			};
			return palette[ mNoteColours.size() % std::size( palette ) ];
		}

		void applyMode( NoteListPanel::Mode mode ) {
			mMode = mode;
			mNoteListPanel.setMode( mode );
			mBtnPro.setToggleState(    mode == NoteListPanel::Mode::Pro,    ::juce::dontSendNotification );
			mBtnMelody.setToggleState( mode == NoteListPanel::Mode::Melody, ::juce::dontSendNotification );
			mBtnDrums.setToggleState(  mode == NoteListPanel::Mode::Drums,  ::juce::dontSendNotification );
		}

		void changeListenerCallback( ::juce::ChangeBroadcaster *source ) override {
			if ( mActiveSelector != nullptr && source == mActiveSelector.getComponent() && mEditingIndex >= 0 ) {
				mNoteColours[ mEditingIndex ] = mActiveSelector->getCurrentColour();
			}
		}

		void openColourPicker( int index, ::juce::Rectangle<int> screenBounds ) {
			if ( index < 0 || index >= static_cast<int>( mNoteColours.size() ) )
				return;
			if ( mActiveSelector != nullptr )
				mActiveSelector->removeChangeListener( this );

			auto selector = std::make_unique<::juce::ColourSelector>( ::juce::ColourSelector::showColourAtTop
			                                                          | ::juce::ColourSelector::showSliders
			                                                          | ::juce::ColourSelector::showColourspace );
			selector->setSize( 300, 380 );
			selector->setCurrentColour( mNoteColours[ index ] );
			selector->addChangeListener( this );

			mActiveSelector = selector.get();
			mEditingIndex   = index;

			::juce::CallOutBox::launchAsynchronously( std::move( selector ), screenBounds, nullptr );
		}

		void resized() override {
			constexpr int panel_w  = 240;
			constexpr int top_h    = 50;
			constexpr int btn_y    = 10;
			constexpr int btn_h    = 30;
			constexpr int btn_w    = 60;
			constexpr int btn_gap  = 4;
			constexpr int btns_x   = 220;
			const int     w        = getWidth();
			const int     h        = getHeight();
			const int     half_h   = ( h - top_h ) / 2;

			mBtnPro.setBounds(    btns_x,                        btn_y, btn_w, btn_h );
			mBtnMelody.setBounds( btns_x + btn_w + btn_gap,      btn_y, btn_w, btn_h );
			mBtnDrums.setBounds(  btns_x + 2 * ( btn_w + btn_gap ), btn_y, btn_w, btn_h );

			mOrbitalDisplay.setBounds( 0, top_h, w - panel_w, h - top_h );
			mNoteListPanel.setBounds( w - panel_w, top_h, panel_w, half_h );
			mCcListPanel.setBounds( w - panel_w, top_h + half_h, panel_w, h - top_h - half_h );
		}

		void timerCallback() override {
			const int64_t currentTime    = mPluginInstanceRef.getTime();
			const float   bpm            = mPluginInstanceRef.getBpm();
			const float   sampleRate     = static_cast<float>( mPluginInstanceRef.getSampleRate() );
			const auto   &notes          = mPluginInstanceRef.getNotes();
			const float   samplesPerBeat = sampleRate * 60.0f / bpm;
			const float   currentBeat    = static_cast<float>( currentTime ) / samplesPerBeat;
			const float   lastBeat       = static_cast<float>( mLastTime ) / samplesPerBeat;

			// Compute pitch range for radius normalisation
			int minPitch = 127, maxPitch = 0;
			for ( const auto &note : notes ) {
				minPitch = std::min( minPitch, note.pitch );
				maxPitch = std::max( maxPitch, note.pitch );
			}
			const int pitchRange = maxPitch - minPitch;

			std::vector<OrbitalDisplay::VoiceState> states;
			states.reserve( static_cast<size_t>( notes.size() ) );
			for ( const auto &note : notes ) {
				const float phase       = std::fmod( currentBeat, note.period ) / note.period - note.offset / note.period;
				const bool  triggered   = std::floor( ( currentBeat - note.offset ) / note.period )
				                          > std::floor( ( lastBeat - note.offset ) / note.period );
				const float trackRadius = pitchRange > 0 ? static_cast<float>( note.pitch - minPitch ) / pitchRange : 0.5f;
				states.push_back( { phase, triggered, trackRadius } );
			}

			mOrbitalDisplay.setVoices( std::move( states ) );
			mOrbitalDisplay.setColours( mNoteColours );
			mOrbitalDisplay.repaint();

			mNoteListPanel.setNotes( { notes.begin(), notes.end() } );
			mNoteListPanel.setColours( mNoteColours );
			mNoteListPanel.repaint();

			const auto &ccs = mPluginInstanceRef.getCCs();
			mCcListPanel.setCCs( { ccs.begin(), ccs.end() } );
			mCcListPanel.repaint();

			mTimeDisplay.setTime( currentTime );
			mTimeDisplay.repaint();

			mLastTime = currentTime;
		}
	};

	p_loops_ui::p_loops_ui( ::plop::p_loops::p_loops &owner ) :
			  ::juce::AudioProcessorEditor( owner ), mPluginInstanceRef( owner ) {
		// Assign initial colours for pre-loaded notes
		for ( size_t i = 0; i < owner.getNotes().size(); ++i )
			mNoteColours.push_back( nextPaletteColour() );

		// Mode buttons
		for ( auto *btn : { &mBtnPro, &mBtnMelody, &mBtnDrums } ) {
			btn->setClickingTogglesState( false );
			btn->setColour( ::juce::TextButton::buttonOnColourId, ::juce::Colour( 0xff4fc3f7 ) );
			addAndMakeVisible( *btn );
		}
		mBtnPro.onClick    = [ this ] { applyMode( NoteListPanel::Mode::Pro );    };
		mBtnMelody.onClick = [ this ] { applyMode( NoteListPanel::Mode::Melody ); };
		mBtnDrums.onClick  = [ this ] { applyMode( NoteListPanel::Mode::Drums );  };
		applyMode( NoteListPanel::Mode::Melody ); // set initial highlight

		mNoteListPanel.onColourSwatchClicked = [ this ]( int index, ::juce::Rectangle<int> screenBounds ) {
			openColourPicker( index, screenBounds );
		};

		mNoteListPanel.onAddNote = [ this ] {
			const bool isDrums = ( mMode == NoteListPanel::Mode::Drums );
			PeriodicNote defaultNote{ .pitch    = isDrums ? 36 : 60,
			                          .period   = 1.0f,
			                          .offset   = 0.0f,
			                          .duration = 0.5f,
			                          .channel  = isDrums ? 9 : 0 };
			mPluginInstanceRef.addNote( defaultNote );
			mNoteColours.push_back( nextPaletteColour() );
		};

		mNoteListPanel.onNoteChanged = [ this ]( int index, PeriodicNote note ) {
			mPluginInstanceRef.updateNote( index, note );
		};

		mNoteListPanel.onRemoveNote = [ this ]( int index ) {
			mPluginInstanceRef.removeNote( index );
			if ( index < static_cast<int>( mNoteColours.size() ) )
				mNoteColours.erase( mNoteColours.begin() + index );
		};

		mCcListPanel.onAddCc = [ this ] {
			constexpr PeriodicCC defaultCc{ .number = 1, .period = 1.0f, .offset = 0.0f, .channel = 0 };
			mPluginInstanceRef.addCc( defaultCc );
		};

		mCcListPanel.onCcChanged = [ this ]( int index, PeriodicCC cc ) { mPluginInstanceRef.updateCc( index, cc ); };

		mCcListPanel.onRemoveCc = [ this ]( int index ) { mPluginInstanceRef.removeCc( index ); };

		addAndMakeVisible( mTimeDisplay );
		mTimeDisplay.setBounds( 10, 10, 200, 30 );

		addAndMakeVisible( mOrbitalDisplay );
		mNoteListPanel.setShowChannel( owner.wrapperType == ::juce::AudioProcessor::wrapperType_Standalone );
		addAndMakeVisible( mNoteListPanel );
		addAndMakeVisible( mCcListPanel );

		setSize( 800, 600 ); // triggers resized()

		setResizable( true, true );
		startTimerHz( 30 );
	}

	p_loops_ui::~p_loops_ui() {
		if ( mActiveSelector != nullptr )
			mActiveSelector->removeChangeListener( this );
		stopTimer();
	}

} // namespace plop::ui

#endif // PLOP_SRC_UI_P_LOOPS_UI_HPP
