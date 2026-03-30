#ifndef PLOP_SRC_UI_P_LOOPS_UI_HPP
#define PLOP_SRC_UI_P_LOOPS_UI_HPP

#include <cmath>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "music/scales.hpp"
#include "processor/periodic_loops.hpp"
#include "ui/cc_display.hpp"
#include "ui/colours.hpp"
#include "ui/ui_constants.hpp"
#include "ui/cc_list_panel.hpp"
#include "ui/midi_export_button.hpp"
#include "ui/note_list_panel.hpp"
#include "ui/orbital_display.hpp"
#include "ui/pattern_picker.hpp"
#include "ui/settings_panel.hpp"
#include "ui/time_display.hpp"

namespace plop::ui {

	class PLoopsUi
			  : public ::juce::AudioProcessorEditor
			  , private ::juce::Timer
			  , private ::juce::ChangeListener {
	 private:
		PlopLookAndFeel          mLookAndFeel;
		::plop::p_loops::PLoops &mPluginInstanceRef;
		TimeDisplay              mTimeDisplay;
		OrbitalDisplay           mOrbitalDisplay;
		NoteListPanel            mNoteListPanel;
		CcListPanel              mCcListPanel;
		CcDisplay                mCcDisplay;
		MidiExportButton         mMidiExportButton;
		PatternPicker            mPatternPicker;
		SettingsPanel            mSettingsPanel;
		::juce::TextButton       mBtnPlayPause{ "Play" };
		bool                     mIsStandalone = false;
		int64_t                  mLastTime     = 0;
		int                      mLastCcCount  = -1;

		PluginMode mMode = PluginMode::melody;

		std::vector<::juce::Colour>                            mNoteColours;
		::juce::Component::SafePointer<::juce::ColourSelector> mActiveSelector;
		int                                                    mEditingIndex = -1;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( PLoopsUi )

	 public:
		explicit PLoopsUi( ::plop::p_loops::PLoops &owner );
		~PLoopsUi();

	 private:
		::juce::Colour nextPaletteColour() const {
			static const ::juce::Colour palette[] = {
				colours::paletteBlue,   colours::paletteRed,  colours::paletteGreen, colours::paletteOrange,
				colours::palettePurple, colours::paletteTeal, colours::palettePink,  colours::paletteBrown,
			};
			return palette[ mNoteColours.size() % std::size( palette ) ];
		}

		void applyMode( PluginMode mode ) {
			mMode = mode;
			mNoteListPanel.setMode( mode );
			mSettingsPanel.setMode( mode );
			mPluginInstanceRef.setMode( mode );

			resized(); // settings panel height may change per mode
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
			constexpr int panelW = 320;
			constexpr int top_h  = 50;
			constexpr int btn_y  = 10;
			constexpr int btn_h  = 30;
			const int     w      = getWidth();
			const int     h      = getHeight();

			// ---- Top bar ----
			mTimeDisplay.setBounds( 10, btn_y, 200, btn_h );
			mMidiExportButton.setBounds( w - 90, btn_y, 80, btn_h );

			constexpr int PP_W    = 70;
			constexpr int PP_GAP  = 6;
			const int     ppRight = mIsStandalone ? w - 90 - PP_GAP - PP_W : w - 90;
			if ( mIsStandalone )
				mBtnPlayPause.setBounds( ppRight, btn_y, PP_W, btn_h );

			const int pickerX = 220;
			const int pickerW = ppRight - pickerX - PP_GAP;
			mPatternPicker.setBounds( pickerX, btn_y, pickerW, btn_h );

			// ---- Left side: orbital + cc display ----
			const int ccDisplayH = mCcDisplay.getPreferredHeight();
			const int orbitalH   = ::juce::jmax( 0, h - top_h - ccDisplayH );
			mOrbitalDisplay.setBounds( 0, top_h, w - panelW, orbitalH );
			mCcDisplay.setBounds( 0, top_h + orbitalH, w - panelW, ccDisplayH );

			// ---- Right column: settings / notes / cc ----
			const int settingsH = mSettingsPanel.getPreferredHeight();
			const int rightY    = top_h;
			mSettingsPanel.setBounds( w - panelW, rightY, panelW, settingsH );

			const int ccPanelH = mCcListPanel.isCollapsed() ? mCcListPanel.getCollapsedHeight() : ( h - rightY - settingsH ) / 2;
			const int notePanelH = h - rightY - settingsH - ccPanelH;
			mNoteListPanel.setBounds( w - panelW, rightY + settingsH, panelW, notePanelH );
			mCcListPanel.setBounds( w - panelW, rightY + settingsH + notePanelH, panelW, ccPanelH );
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
			if ( ccCount != mLastCcCount ) {
				mLastCcCount = ccCount;
				resized();
			}

			mTimeDisplay.setTime( currentTime );
			mTimeDisplay.repaint();
			mLastTime = currentTime;
		}
	};

	PLoopsUi::PLoopsUi( ::plop::p_loops::PLoops &owner ) :
			  ::juce::AudioProcessorEditor( owner ),
			  mPluginInstanceRef( owner ),
			  mNoteListPanel( {
				 .onColourSwatchClicked = [ this ]( int i, ::juce::Rectangle<int> sb ) { openColourPicker( i, sb ); },
				 .onRemoveNote =
					[ this ]( int i ) {
						mPluginInstanceRef.removeNote( i );
						if ( i < static_cast<int>( mNoteColours.size() ) )
							mNoteColours.erase( mNoteColours.begin() + i );
					},
				 .onAddNote =
					[ this ] {
						const bool  isDrums  = ( mMode == PluginMode::drums );
						const bool  isSilica = ( mMode == PluginMode::silica );
						const float period   = isSilica ? mPluginInstanceRef.getSilicaPeriod() : 1.0f;
						int         pitch    = isDrums ? 36 : 60;
						if ( mMode == PluginMode::scale ) {
							const auto &pc = music::SCALES[ static_cast<size_t>( mPluginInstanceRef.getScaleType() ) ].pitchClasses;
							pitch = music::snapToScale( pitch, mPluginInstanceRef.getScaleRoot(), pc );
						}
						PeriodicNote note{
							.pitch = pitch, .period = period, .offset = 0.0f, .duration = 0.5f, .channel = isDrums ? 9 : 0
						};
						mPluginInstanceRef.addNote( note );
						mNoteColours.push_back( nextPaletteColour() );
					},
				 .onNoteChanged = [ this ]( int i, PeriodicNote n ) { mPluginInstanceRef.updateNote( i, n ); },
			  } ),
			  mCcListPanel( {
									.onRemoveCc = [ this ]( int i ) { mPluginInstanceRef.removeCc( i ); },
									.onAddCc =
									  [ this ] {
										  constexpr PeriodicCC defaultCc{ .number = 1, .period = 1.0f, .offset = 0.0f, .channel = 0 };
										  mPluginInstanceRef.addCc( defaultCc );
									  },
									.onCcChanged = [ this ]( int i, PeriodicCC cc ) { mPluginInstanceRef.updateCc( i, cc ); },
								 },
	                      [ this ] { resized(); } ),
			  mMidiExportButton( [ this ] {
				  return generateMidiExport(
					 mPluginInstanceRef.getNotes(), mPluginInstanceRef.getCCs(), mPluginInstanceRef.getBpm() );
			  } ),
			  mPatternPicker( [ this ]( const std::vector<PeriodicNote> &notes, bool add ) {
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
			  } ),
			  mSettingsPanel( {
				 .onModeChanged         = [ this ]( PluginMode mode ) { applyMode( mode ); },
				 .onSilicaPeriodChanged = [ this ]( float period ) { mPluginInstanceRef.setSilicaPeriod( period ); },
				 .onScaleChanged =
					[ this ]( int root, int typeIdx ) {
						mPluginInstanceRef.setScaleRoot( root );
						mPluginInstanceRef.setScaleType( typeIdx );
						mNoteListPanel.setScaleConstraint( root, typeIdx );
						if ( mMode == PluginMode::scale ) {
							const auto &pc    = music::SCALES[ static_cast<size_t>( typeIdx ) ].pitchClasses;
							const auto &notes = mPluginInstanceRef.getNotes();
							for ( int i = 0; i < static_cast<int>( notes.size() ); ++i ) {
								const int snapped = music::snapToScale( notes[ i ].pitch, root, pc );
								if ( snapped != notes[ i ].pitch ) {
									auto updated  = notes[ i ];
									updated.pitch = snapped;
									mPluginInstanceRef.updateNote( i, updated );
								}
							}
						}
					},
			  } ),
			  mIsStandalone( owner.wrapperType == ::juce::AudioProcessor::wrapperType_Standalone ) {
		for ( size_t i = 0; i < owner.getNotes().size(); ++i )
			mNoteColours.push_back( nextPaletteColour() );

		// Restore mode from processor state
		const auto initialMode = owner.getMode();
		mMode                  = initialMode;
		mNoteListPanel.setMode( initialMode );
		mSettingsPanel.setMode( initialMode );
		mSettingsPanel.setSilicaPeriod( owner.getSilicaPeriod() );
		mSettingsPanel.setScaleRoot( owner.getScaleRoot() );
		mSettingsPanel.setScaleType( owner.getScaleType() );
		mNoteListPanel.setScaleConstraint( owner.getScaleRoot(), owner.getScaleType() );

		if ( mIsStandalone ) {
			const bool playing = owner.isStandalonePlaying();
			mBtnPlayPause.setColour( ::juce::TextButton::buttonOnColourId, colours::btnAccentColourAlt );
			mBtnPlayPause.setClickingTogglesState( false );
			mBtnPlayPause.setButtonText( playing ? "Pause" : "Play" );
			mBtnPlayPause.setToggleState( playing, ::juce::dontSendNotification );
			mBtnPlayPause.onClick = [ this ] {
				const bool next = !mPluginInstanceRef.isStandalonePlaying();
				mPluginInstanceRef.setStandalonePlaying( next );
				mBtnPlayPause.setButtonText( next ? "Pause" : "Play" );
				mBtnPlayPause.setToggleState( next, ::juce::dontSendNotification );
			};
		}

		setLookAndFeel( &mLookAndFeel );

		addAndMakeVisible( mTimeDisplay );
		addAndMakeVisible( mOrbitalDisplay );
		addAndMakeVisible( mCcDisplay );
		mNoteListPanel.setShowChannel( mIsStandalone );
		addAndMakeVisible( mNoteListPanel );
		addAndMakeVisible( mCcListPanel );
		addAndMakeVisible( mMidiExportButton );
		addAndMakeVisible( mPatternPicker );
		addAndMakeVisible( mSettingsPanel );
		if ( mIsStandalone )
			addAndMakeVisible( mBtnPlayPause );

		setSize( 800, 600 );
		setResizable( true, true );
		startTimerHz( 30 );
	}

	PLoopsUi::~PLoopsUi() {
		setLookAndFeel( nullptr );
		if ( mActiveSelector != nullptr )
			mActiveSelector->removeChangeListener( this );
		stopTimer();
	}

} // namespace plop::ui

#endif // PLOP_SRC_UI_P_LOOPS_UI_HPP
