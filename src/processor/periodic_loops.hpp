#ifndef PLOP_SRC_PROCSSOR_PERIODIC_LOOPS_HPP
#define PLOP_SRC_PROCSSOR_PERIODIC_LOOPS_HPP

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
			return m_ui_notes;
		}

		/// Add a note. Must be called from the message thread only.
		void addNote( const PeriodicNote &note );

		/// Remove the note at the given index. Must be called from the message thread only.
		void removeNote( int index );

		/// Replace the note at the given index. Must be called from the message thread only.
		void updateNote( int index, const PeriodicNote &note );

	 private:
		// ---- Audio-safe double buffer -----------------------------------------
		// The UI thread writes to the inactive buffer then atomically promotes it.
		// The audio thread only reads from the active buffer — no locks, no allocs.
		static constexpr int MAX_NOTES = 32;

		struct NoteBuffer {
			std::array<PeriodicNote, MAX_NOTES> notes{};
			int                                 count = 0;
		};

		NoteBuffer       m_note_buf[2];
		std::atomic<int> m_active_buf{ 0 };

		/// UI-thread mirror — kept in sync by addNote / removeNote.
		::std::vector<PeriodicNote> m_ui_notes;

		/// Swap helper: copy active → inactive, apply fn, then promote inactive.
		template<typename Fn>
		void swapBuffer( Fn &&fn )
		{
			const int inactive    = 1 - m_active_buf.load( std::memory_order_relaxed );
			m_note_buf[ inactive ] = m_note_buf[ m_active_buf.load( std::memory_order_acquire ) ];
			fn( m_note_buf[ inactive ] );
			m_active_buf.store( inactive, std::memory_order_release );
		}
		// -----------------------------------------------------------------------

		std::atomic<int64_t> time{ 0 };
		double               mSampleRate = 44100.0;
		float                bpm         = 120.0f;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( p_loops )
	};

} // namespace plop::p_loops

#endif // PLOP_SRC_PROCSSOR_PERIODIC_LOOPS_HPP
