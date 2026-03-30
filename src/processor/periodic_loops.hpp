#ifndef PLOP_SRC_PROCESSOR_PERIODIC_LOOPS_HPP
#define PLOP_SRC_PROCESSOR_PERIODIC_LOOPS_HPP

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "engine.hpp"
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
			if ( host_type.isAbletonLive() || host_type.isProTools() )
				return get_bus_layout();
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

		// ---- Delegating accessors -----------------------------------------------

		int64_t getTime() const {
			return mEngine.getTime();
		}
		float getBpm() const {
			return mEngine.getBpm();
		}

		const ::std::vector<PeriodicNote> &getNotes() const {
			return mEngine.getNotes();
		}
		void addNote( const PeriodicNote &note ) {
			mEngine.addNote( note );
		}
		void removeNote( int index ) {
			mEngine.removeNote( index );
		}
		void updateNote( int index, const PeriodicNote &note ) {
			mEngine.updateNote( index, note );
		}

		const ::std::vector<PeriodicCC> &getCCs() const {
			return mEngine.getCCs();
		}
		void addCc( const PeriodicCC &cc ) {
			mEngine.addCc( cc );
		}
		void removeCc( int index ) {
			mEngine.removeCc( index );
		}
		void updateCc( int index, const PeriodicCC &cc ) {
			mEngine.updateCc( index, cc );
		}

		void setSilicaPeriod( float period ) {
			mEngine.setSilicaPeriod( period );
		}
		float getSilicaPeriod() const {
			return mEngine.getSilicaPeriod();
		}

		void setMode( PluginMode mode ) {
			mEngine.setMode( mode );
		}

		PluginMode getMode() const {
			return mEngine.getMode();
		}

		void setScaleRoot( int root ) {
			mEngine.setScaleRoot( root );
		}
		int getScaleRoot() const {
			return mEngine.getScaleRoot();
		}
		void setScaleType( int typeIndex ) {
			mEngine.setScaleType( typeIndex );
		}
		int getScaleType() const {
			return mEngine.getScaleType();
		}

		void setStandalonePlaying( bool playing ) {
			mEngine.setStandalonePlaying( playing );
		}
		bool isStandalonePlaying() const {
			return mEngine.isStandalonePlaying();
		}

	 private:
		Engine mEngine;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( p_loops )
	};

} // namespace plop::p_loops

#endif // PLOP_SRC_PROCESSOR_PERIODIC_LOOPS_HPP
