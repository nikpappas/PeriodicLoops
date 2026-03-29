#ifndef PLOP_SRC_UI_P_LOOPS_UI_HPP
#define PLOP_SRC_UI_P_LOOPS_UI_HPP

#include <cmath>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "processor/periodic_loops.hpp"
#include "ui/cc_display.hpp"
#include "ui/cc_list_panel.hpp"
#include "ui/midi_export_button.hpp"
#include "ui/note_list_panel.hpp"
#include "ui/orbital_display.hpp"
#include "ui/pattern_picker.hpp"
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
		CcDisplay                 mCcDisplay;
		MidiExportButton          mMidiExportButton;
		PatternPicker             mPatternPicker;
		int64_t                   mLastTime    = 0;
		int                       mLastCcCount = -1;

		::juce::TextButton  mBtnPro{ "Pro" };
		::juce::TextButton  mBtnMelody{ "Melody" };
		::juce::TextButton  mBtnDrums{ "Drums" };
		::juce::TextButton  mBtnSilica{ "Silica" };
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
			const bool wasSilica = ( mMode == NoteListPanel::Mode::Silica );
			const bool isSilica  = ( mode == NoteListPanel::Mode::Silica );
			mMode = mode;
			mNoteListPanel.setMode( mode );
			mBtnPro.setToggleState(    mode == NoteListPanel::Mode::Pro,    ::juce::dontSendNotification );
			mBtnMelody.setToggleState( mode == NoteListPanel::Mode::Melody, ::juce::dontSendNotification );
			mBtnDrums.setToggleState(  mode == NoteListPanel::Mode::Drums,  ::juce::dontSendNotification );
			mBtnSilica.setToggleState( isSilica,                            ::juce::dontSendNotification );

			if ( isSilica != wasSilica )
				mPluginInstanceRef.setSilicaMode( isSilica );
		}

		void changeListenerCallback( ::juce::ChangeBroadcaster *source ) override {
			if ( mActiveSelector != nullptr && source == mActiveSelector.getComponent() && mEditingIndex >= 0 )
				mNoteColours[ mEditingIndex ] = mActiveSelector->getCurrentColour();
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

			mBtnPro.setBounds(    btns_x,                           btn_y, btn_w, btn_h );
			mBtnMelody.setBounds( btns_x + btn_w + btn_gap,         btn_y, btn_w, btn_h );
			mBtnDrums.setBounds(  btns_x + 2 * ( btn_w + btn_gap ), btn_y, btn_w, btn_h );
			mBtnSilica.setBounds( btns_x + 3 * ( btn_w + btn_gap ), btn_y, btn_w, btn_h );
			mMidiExportButton.setBounds( w - 90, btn_y, 80, btn_h );

			const int modeEnd     = mBtnSilica.getRight();
			const int exportStart = w - 90;
			mPatternPicker.setBounds( modeEnd + 8, btn_y, exportStart - modeEnd - 16, btn_h );

			const int ccDisplayH = mCcDisplay.getPreferredHeight();
			const int orbitalH   = ::juce::jmax( 0, h - top_h - ccDisplayH );
			mOrbitalDisplay.setBounds( 0, top_h, w - panel_w, orbitalH );
			mCcDisplay.setBounds( 0, top_h + orbitalH, w - panel_w, ccDisplayH );
			const int ccPanelH  = mCcListPanel.isCollapsed() ? mCcListPanel.getCollapsedHeight() : ( h - top_h ) / 2;
			const int notePanelH = h - top_h - ccPanelH;
			mNoteListPanel.setBounds( w - panel_w, top_h, panel_w, notePanelH );
			mCcListPanel.setBounds(   w - panel_w, top_h + notePanelH, panel_w, ccPanelH );
		}

		void timerCallback() override {
			const int64_t currentTime    = mPluginInstanceRef.getTime();
			const float   bpm            = mPluginInstanceRef.getBpm();
			const float   sampleRate     = static_cast<float>( mPluginInstanceRef.getSampleRate() );
			const auto   &notes          = mPluginInstanceRef.getNotes();
			const float   samplesPerBeat = sampleRate * 60.0f / bpm;
			const float   currentBeat    = static_cast<float>( currentTime ) / samplesPerBeat;
			const float   lastBeat       = static_cast<float>( mLastTime ) / samplesPerBeat;

			int minPitch = 127, maxPitch = 0;
			for ( const auto &note : notes ) {
				minPitch = std::min( minPitch, note.pitch );
				maxPitch = std::max( maxPitch, note.pitch );
			}
			const int pitchRange = maxPitch - minPitch;

			std::vector<OrbitalDisplay::VoiceState> states;
			states.reserve( notes.size() );
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

			const auto &ccs     = mPluginInstanceRef.getCCs();
			const int   ccCount = static_cast<int>( ccs.size() );
			mCcListPanel.setCCs( { ccs.begin(), ccs.end() } );
			mCcListPanel.repaint();
			mCcDisplay.setCurrentBeat( currentBeat );
			mCcDisplay.setCCs( { ccs.begin(), ccs.end() } );
			mCcDisplay.repaint();
			if ( ccCount != mLastCcCount ) { mLastCcCount = ccCount; resized(); }

			mTimeDisplay.setTime( currentTime );
			mTimeDisplay.repaint();
			mLastTime = currentTime;
		}
	};

	p_loops_ui::p_loops_ui( ::plop::p_loops::p_loops &owner )
	    : ::juce::AudioProcessorEditor( owner )
	    , mPluginInstanceRef( owner )
	    , mNoteListPanel( {
	          .onColourSwatchClicked = [ this ]( int i, ::juce::Rectangle<int> sb ) { openColourPicker( i, sb ); },
	          .onRemoveNote          = [ this ]( int i ) {
	              mPluginInstanceRef.removeNote( i );
	              if ( i < static_cast<int>( mNoteColours.size() ) )
	                  mNoteColours.erase( mNoteColours.begin() + i );
	          },
	          .onAddNote = [ this ] {
	              const bool   isDrums  = ( mMode == NoteListPanel::Mode::Drums );
	              const bool   isSilica = ( mMode == NoteListPanel::Mode::Silica );
	              const float  period   = isSilica ? mPluginInstanceRef.getSilicaPeriod() : 1.0f;
	              PeriodicNote note{ .pitch = isDrums ? 36 : 60, .period = period, .offset = 0.0f, .duration = 0.5f, .channel = isDrums ? 9 : 0 };
	              mPluginInstanceRef.addNote( note );
	              mNoteColours.push_back( nextPaletteColour() );
	          },
	          .onNoteChanged = [ this ]( int i, PeriodicNote n ) { mPluginInstanceRef.updateNote( i, n ); },
	      } )
	    , mCcListPanel( {
	          .onRemoveCc  = [ this ]( int i ) { mPluginInstanceRef.removeCc( i ); },
	          .onAddCc     = [ this ] {
	              constexpr PeriodicCC defaultCc{ .number = 1, .period = 1.0f, .offset = 0.0f, .channel = 0 };
	              mPluginInstanceRef.addCc( defaultCc );
	          },
	          .onCcChanged = [ this ]( int i, PeriodicCC cc ) { mPluginInstanceRef.updateCc( i, cc ); },
	      }, [ this ] { resized(); } )
	    , mMidiExportButton( [ this ] {
	          return generateMidiExport( mPluginInstanceRef.getNotes(), mPluginInstanceRef.getCCs(), mPluginInstanceRef.getBpm() );
	      } )
	    , mPatternPicker( [ this ]( const std::vector<PeriodicNote> &notes, bool add ) {
	          if ( !add ) {
	              const int n = static_cast<int>( mPluginInstanceRef.getNotes().size() );
	              for ( int i = n - 1; i >= 0; --i )
	                  mPluginInstanceRef.removeNote( i );
	              mNoteColours.clear();
	          }
	          for ( const auto &note : notes ) {
	              mPluginInstanceRef.addNote( note );
	              mNoteColours.push_back( nextPaletteColour() );
	          }
	      } ) {
		for ( size_t i = 0; i < owner.getNotes().size(); ++i )
			mNoteColours.push_back( nextPaletteColour() );

		for ( auto *btn : { &mBtnPro, &mBtnMelody, &mBtnDrums, &mBtnSilica } ) {
			btn->setClickingTogglesState( false );
			btn->setColour( ::juce::TextButton::buttonOnColourId, ::juce::Colour( 0xff4fc3f7 ) );
			addAndMakeVisible( *btn );
		}
		mBtnSilica.setColour( ::juce::TextButton::buttonOnColourId, ::juce::Colour( 0xffab47bc ) );
		mBtnPro.onClick    = [ this ] { applyMode( NoteListPanel::Mode::Pro ); };
		mBtnMelody.onClick = [ this ] { applyMode( NoteListPanel::Mode::Melody ); };
		mBtnDrums.onClick  = [ this ] { applyMode( NoteListPanel::Mode::Drums ); };
		mBtnSilica.onClick = [ this ] { applyMode( NoteListPanel::Mode::Silica ); };
		applyMode( owner.getSilicaMode() ? NoteListPanel::Mode::Silica : NoteListPanel::Mode::Melody );

		addAndMakeVisible( mTimeDisplay );
		mTimeDisplay.setBounds( 10, 10, 200, 30 );
		addAndMakeVisible( mOrbitalDisplay );
		addAndMakeVisible( mCcDisplay );
		mNoteListPanel.setShowChannel( owner.wrapperType == ::juce::AudioProcessor::wrapperType_Standalone );
		addAndMakeVisible( mNoteListPanel );
		addAndMakeVisible( mCcListPanel );
		addAndMakeVisible( mMidiExportButton );
		addAndMakeVisible( mPatternPicker );

		setSize( 800, 600 );
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
