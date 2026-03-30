#ifndef PLOP_SRC_PROCESSOR_ENGINE_HPP
#define PLOP_SRC_PROCESSOR_ENGINE_HPP

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "music/midi.hpp"
#include "plugin_state.hpp"

namespace plop::p_loops {

	class Engine {

	 public:
		Engine();

		void prepare( double sampleRate, int blockSize );
		void reset();

		/// Called from the audio thread. Fills midi with scheduled note-on/off and CC events.
		void process( ::juce::MidiBuffer                 &midi,
		              int                                 numSamples,
		              ::juce::AudioPlayHead              *playHead,
		              ::juce::AudioProcessor::WrapperType wrapperType );

		// ---- Note management (message thread) ------------------------------------

		const ::std::vector<PeriodicNote> &getNotes() const {
			return mUiNotes;
		}

		void addNote( const PeriodicNote &note );
		void removeNote( int index );
		void updateNote( int index, const PeriodicNote &note );

		// ---- CC management (message thread) --------------------------------------

		const ::std::vector<PeriodicCC> &getCCs() const {
			return mUiCcs;
		}

		void addCc( const PeriodicCC &cc );
		void removeCc( int index );
		void updateCc( int index, const PeriodicCC &cc );

		// ---- Silica mode ---------------------------------------------------------

		bool isSilicaMode();

		void  setSilicaPeriod( float period );
		float getSilicaPeriod() const {
			return mSilicaPeriod;
		}

		// ---- UI mode / scale state -----------------------------------------------

		void setMode( PluginMode mode );

		PluginMode getMode() const;

		void setScaleRoot( int root ) {
			mScaleRoot = root;
		}
		int getScaleRoot() const {
			return mScaleRoot;
		}

		void setScaleType( int typeIndex ) {
			mScaleType = typeIndex;
		}
		int getScaleType() const {
			return mScaleType;
		}

		// ---- Playback state ------------------------------------------------------

		void setStandalonePlaying( bool playing );
		bool isStandalonePlaying() const {
			return mStandalonePlaying.load( ::std::memory_order_acquire );
		}
		int64_t getTime() const {
			return mTime.load();
		}
		float getBpm() const {
			return mBpm.load();
		}

		// ---- State serialisation (message thread) --------------------------------

		PluginState captureState() const;
		void        applyState( const PluginState &state );

	 private:
		// ---- Audio-safe double buffer --------------------------------------------
		static constexpr int MAX_NOTES = 32;

		struct NoteBuffer {
			::std::array<PeriodicNote, MAX_NOTES> notes{};
			int                                   count = 0;
		};

		struct CCBuffer {
			::std::array<PeriodicCC, MAX_NOTES> cc{};
			int                                 count = 0;
		};

		NoteBuffer         mNoteBuf[ 2 ];
		::std::atomic<int> mActiveNoteBuf{ 0 };

		CCBuffer           mCcBuf[ 2 ];
		::std::atomic<int> mActiveCcBuf{ 0 };

		::std::vector<PeriodicNote> mUiNotes;
		::std::vector<PeriodicCC>   mUiCcs;

		template <typename Fn>
		void swapNoteBuffer( Fn &&fn ) {
			const int inactive   = 1 - mActiveNoteBuf.load( ::std::memory_order_relaxed );
			mNoteBuf[ inactive ] = mNoteBuf[ mActiveNoteBuf.load( ::std::memory_order_acquire ) ];
			fn( mNoteBuf[ inactive ] );
			mActiveNoteBuf.store( inactive, ::std::memory_order_release );
		}

		template <typename Fn>
		void swapCCBuffer( Fn &&fn ) {
			const int inactive = 1 - mActiveCcBuf.load( ::std::memory_order_relaxed );
			mCcBuf[ inactive ] = mCcBuf[ mActiveCcBuf.load( ::std::memory_order_acquire ) ];
			fn( mCcBuf[ inactive ] );
			mActiveCcBuf.store( inactive, ::std::memory_order_release );
		}

		// -------------------------------------------------------------------------

		PluginMode mMode         = PluginMode::melody;
		float      mSilicaPeriod = 4.0f;
		int        mScaleRoot    = 0; // 0 = C
		int        mScaleType    = 1; // index into music::SCALES (1 = Major)

		void redistributeSilicaOffsets();

		::std::atomic<bool>    mStandalonePlaying{ true };
		::std::atomic<bool>    mSendAllNotesOff{ false };
		::std::atomic<int64_t> mTime{ 0 };
		::std::atomic<float>   mBpm{ 120.0f };
		double                 mSampleRate = 44100.0;
		int                    mBlockSize  = 128;
	};

} // namespace plop::engine

#endif // PLOP_SRC_PROCESSOR_ENGINE_HPP
