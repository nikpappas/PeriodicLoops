#ifndef PLOP_SRC_PROCESSOR_PERIODIC_LOOPS_HPP
#define PLOP_SRC_PROCESSOR_PERIODIC_LOOPS_HPP

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "music/midi.hpp"
#include "utils/constants.hpp"
#include "utils/utils.hpp"

namespace plop::p_loops {

	class p_loops : public ::juce::AudioProcessor {
	 public:
		p_loops();

		~p_loops() noexcept override;

		static BusesProperties get_bus_layout() {
			return BusesProperties()
			  .withInput( "Input", juce::AudioChannelSet::stereo(), true )
			  .withOutput( "Output", juce::AudioChannelSet::stereo(), true );
		}

		static BusesProperties get_midifx_bus_layout() {
			const auto host_type = ::juce::PluginHostType();
			if ( host_type.isAbletonLive() || host_type.isProTools() ) {
				return get_bus_layout();
			}

			return BusesProperties();
		}

		bool isBusesLayoutSupported( const BusesLayout &layouts ) const override;

		void prepareToPlay( double newSampleRate, int samplesPerBlock ) override;

		void releaseResources() override;

		void reset() override;

		void processBlock( ::juce::AudioBuffer<float> &ab, ::juce::MidiBuffer &mb ) override;
		void processBlock( ::juce::AudioBuffer<double> &ab, ::juce::MidiBuffer &mb ) override;

		bool hasEditor() const override {
			return true;
		}

		::juce::AudioProcessorEditor *createEditor() override;

		const ::juce::String getName() const override {
			return utils::to_juce_string( ::plop::utils::PLUGIN_NAME );
		}

		bool acceptsMidi() const override {
			return true;
		}

		bool producesMidi() const override {
			return true;
		}

		bool isMidiEffect() const override {
			return true;
		}

		double getTailLengthSeconds() const override {
			return 5.0;
		}

		int getNumPrograms() override {
			return 1;
		}

		int getCurrentProgram() override {
			return 0;
		}

		void setCurrentProgram( int ) override {
		}

		const ::juce::String getProgramName( int ) override {
			return "None";
		}

		void changeProgramName( int, const ::juce::String & ) override {
		}

		void getStateInformation( ::juce::MemoryBlock &destData ) override;

		void setStateInformation( const void *data, int sizeInBytes ) override;

		static ::juce::File log_file();

		int64_t getTime() const {
			return time.load();
		}

		float getBpm() const {
			return bpm;
		}

		/// Returns the UI-thread view of the note list. Always called from the message thread.
		const ::std::vector<PeriodicNote> &getNotes() const {
			return mUiNotes;
		}

		/// Add a note. Must be called from the message thread only.
		void addNote( const PeriodicNote &note );

		/// Remove the note at the given index. Must be called from the message thread only.
		void removeNote( int index );

		/// Replace the note at the given index. Must be called from the message thread only.
		void updateNote( int index, const PeriodicNote &note );

		/// Enable/disable Silica mode. In Silica mode all notes share one period
		/// and offsets are equally distributed: offset[i] = i * period / N.
		void setSilicaMode( bool enabled );
		bool getSilicaMode() const { return mSilicaMode; }

		void setSilicaPeriod( float period );
		float getSilicaPeriod() const { return mSilicaPeriod; }

		/// Active UI mode (persisted for save/restore).
		void setMode( int mode ) { mMode = mode; }
		int  getMode() const { return mMode; }

		/// Scale mode state (UI-thread only, persisted for save/restore).
		void setScaleRoot( int root ) { mScaleRoot = root; }
		int  getScaleRoot() const { return mScaleRoot; }
		void setScaleType( int typeIndex ) { mScaleType = typeIndex; }
		int  getScaleType() const { return mScaleType; }

		/// Standalone play/pause control.
		void setStandalonePlaying( bool playing );
		bool isStandalonePlaying() const { return mStandalonePlaying.load( std::memory_order_acquire ); }

		/// Returns the UI-thread view of the CC list. Always called from the message thread.
		const ::std::vector<PeriodicCC> &getCCs() const {
			return mUiCcs;
		}

		/// Add a CC. Must be called from the message thread only.
		void addCc( const PeriodicCC &cc );

		/// Remove the CC at the given index. Must be called from the message thread only.
		void removeCc( int index );

		/// Replace the CC at the given index. Must be called from the message thread only.
		void updateCc( int index, const PeriodicCC &cc );

	 private:
		// ---- Audio-safe double buffer -----------------------------------------
		// The UI thread writes to the inactive buffer then atomically promotes it.
		// The audio thread only reads from the active buffer — no locks, no allocs.
		static constexpr int MAX_NOTES = 32;

		struct NoteBuffer {
			std::array<PeriodicNote, MAX_NOTES> notes{};
			int                                 count = 0;
		};

		struct CCBuffer {
			std::array<PeriodicCC, MAX_NOTES> cc{};
			int                               count = 0;
		};

		NoteBuffer       mNoteBuf[ 2 ];
		std::atomic<int> mActiveNoteBuf{ 0 };

		CCBuffer         mCcBuf[ 2 ];
		std::atomic<int> mActiveCcBuf{ 0 };

		/// UI-thread mirror — kept in sync by addNote / removeNote.
		::std::vector<PeriodicNote> mUiNotes;

		/// UI-thread mirror — kept in sync by addCc / removeCc.
		::std::vector<PeriodicCC> mUiCcs;

		/// Swap helper: copy active → inactive, apply fn, then promote inactive.
		template <typename Fn>
		void swapBuffer( Fn &&fn ) {
			const int inactive   = 1 - mActiveNoteBuf.load( std::memory_order_relaxed );
			mNoteBuf[ inactive ] = mNoteBuf[ mActiveNoteBuf.load( std::memory_order_acquire ) ];
			fn( mNoteBuf[ inactive ] );
			mActiveNoteBuf.store( inactive, std::memory_order_release );
		}

		/// Swap helper: copy active → inactive, apply fn, then promote inactive.
		template <typename Fn>
		void swapCCBuffer( Fn &&fn ) {
			const int inactive = 1 - mActiveCcBuf.load( std::memory_order_relaxed );
			mCcBuf[ inactive ] = mCcBuf[ mActiveCcBuf.load( std::memory_order_acquire ) ];
			fn( mCcBuf[ inactive ] );
			mActiveCcBuf.store( inactive, std::memory_order_release );
		}

		// -----------------------------------------------------------------------

		/// Active UI mode (stored as int to avoid depending on UI enum)
		int mMode = 1; // 1 = Melody

		/// Silica mode: equal-offset distribution
		bool  mSilicaMode   = false;
		float mSilicaPeriod = 4.0f;

		/// Scale mode state
		int mScaleRoot = 0;  // 0 = C
		int mScaleType = 1;  // index into music::k_scales (1 = Major)

		/// Recalculate all note offsets and periods for Silica distribution.
		void redistributeSilicaOffsets();

		std::atomic<bool> mStandalonePlaying{ true };
		std::atomic<bool> mSendAllNotesOff{ false };

		std::atomic<int64_t> time{ 0 };
		double               mSampleRate = 44100.0;
		int               mBlockSize  = 124;
		std::atomic<float>   bpm{ 120.0f };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( p_loops )
	};

} // namespace plop::p_loops

#endif // PLOP_SRC_PROCESSOR_PERIODIC_LOOPS_HPP
