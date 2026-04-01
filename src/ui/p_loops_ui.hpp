#ifndef PLOP_SRC_UI_P_LOOPS_UI_HPP
#define PLOP_SRC_UI_P_LOOPS_UI_HPP

#include <cmath>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "music/patterns.hpp"
#include "music/scales.hpp"
#include "processor/periodic_loops.hpp"
#include "ui/cc_controls.hpp"
#include "ui/cc_display.hpp"
#include "ui/cc_list_panel.hpp"
#include "ui/circle_grid.hpp"
#include "ui/colours.hpp"
#include "ui/group_list_panel.hpp"
#include "ui/midi_export_button.hpp"
#include "ui/note_list_panel.hpp"
#include "ui/orbital_display.hpp"
#include "ui/pattern_picker.hpp"
#include "ui/settings_panel.hpp"
#include "ui/time_display.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	class PLoopsUi
			  : public ::juce::AudioProcessorEditor
			  , private ::juce::Timer
			  , private ::juce::ChangeListener {
	 private:
		PlopLookAndFeel          mLookAndFeel;
		::plop::p_loops::PLoops &mPluginInstanceRef;
		OrbitalDisplay           mOrbitalDisplay;
		NoteListPanel            mNoteListPanel;
		GroupListPanel           mGroupListPanel;
		CcListPanel              mCcListPanel;
		CircleGrid               mCircleGrid;
		CcControls               mCcControls;
		CcDisplay                mCcDisplay;
		MidiExportButton         mMidiExportButton;
		PatternPicker            mPatternPicker;
		SettingsPanel            mSettingsPanel;
		::juce::TextButton       mBtnPlayPause{ "Play" };
		bool                     mIsStandalone = false;
		int64_t                  mLastTime     = 0;
		int                      mLastCcCount  = -1;

		PluginMode mMode = PluginMode::Melody;

		std::vector<::juce::Colour>                            mNoteColours;
		::juce::Component::SafePointer<::juce::ColourSelector> mActiveSelector;
		int                                                    mEditingIndex = -1;

		// Groups (owned by UI, flattened to engine on change)
		::std::vector<NoteGroup> mGroups;

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

		bool isGroupMode() const {
			return mMode == PluginMode::Drums || mMode == PluginMode::Silica || mMode == PluginMode::Melody;
		}

		void applyMode( PluginMode mode ) {
			mMode = mode;
			mNoteListPanel.setMode( mode );
			mSettingsPanel.setMode( mode );
			mPluginInstanceRef.setMode( mode );

			// Show/hide panels based on mode
			mNoteListPanel.setVisible( !isGroupMode() );
			mGroupListPanel.setVisible( isGroupMode() );

			if ( isGroupMode() ) {
				// Regenerate voices for silica/melody modes
				regenerateGroupVoices();
				pushGroupsToEngine();
			}

			resized();
		}

		void regenerateGroupVoices() {
			const auto &scalePc = music::SCALES[ static_cast<size_t>( mPluginInstanceRef.getScaleType() ) ].pitchClasses;
			const int   root    = mPluginInstanceRef.getScaleRoot();

			for ( auto &g : mGroups ) {
				if ( g.mode == PluginMode::Silica || g.mode == PluginMode::Melody ) {
					g.voices = music::generateVoices( g, root, scalePc );
				}
				// Drums mode: voices are manually managed, but ensure channel/period propagation
				if ( g.mode == PluginMode::Drums ) {
					for ( auto &v : g.voices ) {
						v.period  = g.period;
						v.channel = g.channel;
					}
				}
			}
		}

		void pushGroupsToEngine() {
			mPluginInstanceRef.setGroups( mGroups );
			auto flatNotes = music::flattenGroups( mGroups );
			mPluginInstanceRef.replaceAllNotes( flatNotes );
		}

		void onGroupsChanged( const ::std::vector<NoteGroup> &groups ) {
			mGroups = groups;
			regenerateGroupVoices();
			pushGroupsToEngine();
			// Do NOT call mGroupListPanel.setGroups() here — the panel already has
			// the right state, and setGroups() would rebuild panels mid-drag.
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

		void paint( ::juce::Graphics &g ) override {
			g.fillAll( colours::panelBg );
		}

		void resized() override {
			constexpr int panelW = 320;
			constexpr int top_h  = 50;
			constexpr int btn_y  = 10;
			constexpr int btn_h  = 30;
			const int     w      = getWidth();
			const int     h      = getHeight();

			// ---- Top bar ----
			mMidiExportButton.setBounds( w - 90, btn_y, 80, btn_h );

			constexpr int PP_W    = 70;
			const int     ppRight = mIsStandalone ? w - 90 - PAD_SM - PP_W : w - 90;
			if ( mIsStandalone )
				mBtnPlayPause.setBounds( ppRight, btn_y, PP_W, btn_h );

			const int pickerX = 220;
			const int pickerW = ppRight - pickerX - PAD_SM;
			mPatternPicker.setBounds( pickerX, btn_y, pickerW, btn_h );

			// ---- Left side: orbital + cc display ----
			const int ccDisplayH = mCcDisplay.getPreferredHeight();
			const int orbitalH   = ::juce::jmax( 0, h - top_h - ccDisplayH );
			mOrbitalDisplay.setBounds( panelW, top_h, w - 2 * panelW, orbitalH );
			mCcDisplay.setBounds( panelW, top_h + orbitalH, w - 2 * panelW, ccDisplayH );

			// ---- Right column: settings / notes or groups / cc ----
			const int settingsH = mSettingsPanel.getPreferredHeight();
			const int rightY    = top_h;
			mSettingsPanel.setBounds( 0, rightY, panelW, settingsH );

			const int panelH = h - rightY - settingsH;

			if ( isGroupMode() ) {
				mGroupListPanel.setBounds( 0, rightY + settingsH, panelW, panelH );
			} else {
				mNoteListPanel.setBounds( 0, rightY + settingsH, panelW, panelH );
			}
			constexpr int CIRCLE_GRID_H = 120;
			constexpr int CC_CONTROLS_H = 120;
			mCircleGrid.setBounds( w - panelW, rightY, panelW, CIRCLE_GRID_H );
			mCcListPanel.setBounds( w - panelW, rightY + CIRCLE_GRID_H, panelW, panelH - CIRCLE_GRID_H - CC_CONTROLS_H );
			mCcControls.setBounds( w - panelW, rightY + panelH - CC_CONTROLS_H, panelW, CC_CONTROLS_H );
		}

		void timerCallback() override {
			const int64_t currentTime    = mPluginInstanceRef.getTime();
			const float   bpm            = mPluginInstanceRef.getBpm();
			const float   sampleRate     = static_cast<float>( mPluginInstanceRef.getSampleRate() );
			const auto   &notes          = mPluginInstanceRef.getNotes();
			const float   samplesPerBeat = sampleRate * 60.0f / bpm;
			const float   currentBeat    = static_cast<float>( currentTime ) / samplesPerBeat;
			const float   lastBeat       = static_cast<float>( mLastTime ) / samplesPerBeat;

			// Orbital display
			int minPitch = 127, maxPitch = 0;
			for ( const auto &note : notes ) {
				minPitch = std::min( minPitch, note.pitch );
				maxPitch = std::max( maxPitch, note.pitch );
			}
			const int pitchRange = maxPitch - minPitch;

			std::vector<OrbitalDisplay::VoiceState> states;
			states.reserve( notes.size() );

			// Build colours for orbital display from groups when in group mode
			::std::vector<::juce::Colour> orbitalColours;
			if ( isGroupMode() ) {
				for ( const auto &g : mGroups ) {
					if ( g.muted )
						continue;
					bool anySolo = false;
					for ( const auto &gg : mGroups )
						if ( gg.solo ) {
							anySolo = true;
							break;
						}
					if ( anySolo && !g.solo )
						continue;
					for ( size_t vi = 0; vi < g.voices.size(); ++vi )
						orbitalColours.push_back( g.muted ? g.colour.withAlpha( 0.2f ) : g.colour );
				}
			}

			for ( const auto &note : notes ) {
				const float phase       = std::fmod( currentBeat, note.period ) / note.period - note.offset / note.period;
				const bool  triggered   = std::floor( ( currentBeat - note.offset ) / note.period )
				                          > std::floor( ( lastBeat - note.offset ) / note.period );
				const float trackRadius = pitchRange > 0 ? static_cast<float>( note.pitch - minPitch ) / pitchRange : 0.5f;
				states.push_back( { phase, triggered, trackRadius } );
			}

			mOrbitalDisplay.setVoices( std::move( states ) );
			mOrbitalDisplay.setColours( isGroupMode() ? orbitalColours : mNoteColours );
			mOrbitalDisplay.repaint();

			if ( !isGroupMode() ) {
				mNoteListPanel.setNotes( { notes.begin(), notes.end() } );
				mNoteListPanel.setColours( mNoteColours );
				mNoteListPanel.repaint();
			} else {
				// Groups are already pushed; just repaint
				mGroupListPanel.repaint();
			}

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
						PeriodicNote note{ .pitch = 60, .period = 1.0f, .offset = 0.0f, .duration = 0.5f, .channel = 0 };
						mPluginInstanceRef.addNote( note );
						mNoteColours.push_back( nextPaletteColour() );
					},
				 .onNoteChanged = [ this ]( int i, PeriodicNote n ) { mPluginInstanceRef.updateNote( i, n ); },
			  } ),
			  mGroupListPanel( {
				 .onGroupsChanged = [ this ]( const ::std::vector<NoteGroup> &groups ) { onGroupsChanged( groups ); },
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
						mGroupListPanel.setScaleConstraint( root, typeIdx );
						if ( isGroupMode() ) {
							regenerateGroupVoices();
							pushGroupsToEngine();
							mGroupListPanel.setGroups( mGroups );
						}
					},
			  } ),
			  mIsStandalone( owner.wrapperType == ::juce::AudioProcessor::wrapperType_Standalone ) {
		for ( size_t i = 0; i < owner.getNotes().size(); ++i )
			mNoteColours.push_back( nextPaletteColour() );

		// Restore mode from processor state
		const auto initialMode = owner.getMode();
		mMode                  = initialMode;
		mGroups                = owner.getGroups();

		mNoteListPanel.setMode( initialMode );
		mGroupListPanel.setGroups( mGroups );
		mGroupListPanel.setScaleConstraint( owner.getScaleRoot(), owner.getScaleType() );

		mSettingsPanel.setMode( initialMode );
		mSettingsPanel.setSilicaPeriod( owner.getSilicaPeriod() );
		mSettingsPanel.setScaleRoot( owner.getScaleRoot() );
		mSettingsPanel.setScaleType( owner.getScaleType() );
		mNoteListPanel.setScaleConstraint( owner.getScaleRoot(), owner.getScaleType() );

		// Show correct panel
		mNoteListPanel.setVisible( !isGroupMode() );
		mGroupListPanel.setVisible( isGroupMode() );

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

		addAndMakeVisible( mOrbitalDisplay );
		addAndMakeVisible( mCcDisplay );
		mNoteListPanel.setShowChannel( mIsStandalone );
		addAndMakeVisible( mNoteListPanel );
		addAndMakeVisible( mGroupListPanel );
		addAndMakeVisible( mCircleGrid );
		addAndMakeVisible( mCcListPanel );
		addAndMakeVisible( mCcControls );
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
