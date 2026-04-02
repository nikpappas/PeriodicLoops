#include "periodic_loops.hpp"

#include "logging/logging.hpp"
#include "plugin_state.hpp"
#include "ui/p_loops_ui.hpp"
#include "utils/constants.hpp"

#include <cstdint>
#include <juce_core/juce_core.h>
#include <string>

using ::juce::AudioBuffer;
using ::juce::AudioChannelSet;
using ::juce::AudioProcessor;
using ::juce::AudioProcessorEditor;
using ::juce::File;
using ::juce::MemoryBlock;
using ::juce::MidiBuffer;
using ::juce::PluginHostType;

using ::plop::PeriodicCC;

using ::std::initializer_list;
using ::std::string;
using ::std::to_string;

using namespace ::plop::p_loops;

PLoops::PLoops() : AudioProcessor( PLoops::getMidifxBusLayout() ) {

	for ( const PeriodicNote &note : initializer_list<PeriodicNote>{
			  { .pitch = 46, .period = 16.0f, .offset = 0.1f, .duration = 1.0f, .channel = 2 },
			  { .pitch = 36, .period = 2.0f, .offset = 0, .duration = 1.0f, .channel = 2 },
			  { .pitch = 42, .period = 1.0f, .offset = 0.5, .duration = 1.0f, .channel = 2 },

			  { .pitch = 72, .period = 16.0f, .offset = 0, .duration = 2.0f, .channel = 0 },
			  { .pitch = 67, .period = 16.0f, .offset = 2 * 0.33f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 65, .period = 16.0f, .offset = 2 * 0.5f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 66, .period = 16.0f, .offset = 2 * 1.35f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 65, .period = 16.0f, .offset = 2 * 1.35f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 72, .period = 16.0f, .offset = 2 * 2, .duration = 2.0f, .channel = 0 },
			  { .pitch = 67, .period = 16.0f, .offset = 2 * 2.33f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 65, .period = 16.0f, .offset = 2 * 2.5f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 60, .period = 16.0f, .offset = 2 * 3.35f, .duration = 0.5f, .channel = 0 },
			  { .pitch = 63, .period = 16.0f, .offset = 2 * 3.5f, .duration = 0.5f, .channel = 0 },

			  { .pitch = 60, .period = 4.0f, .offset = 2 * 0.0f, .duration = 0.5f, .channel = 1 },
			  { .pitch = 63, .period = 4.0f, .offset = 2 * 1.5f, .duration = 0.5f, .channel = 1 },
			  { .pitch = 65, .period = 4.0f, .offset = 2 * 0.0f, .duration = 0.5f, .channel = 1 },
			  { .pitch = 67, .period = 8.0f, .offset = 2 * 1.25f, .duration = 0.5f, .channel = 1 },
			  { .pitch = 67, .period = 12.0f, .offset = 2 * 0.0f, .duration = 0.5f, .channel = 1 },
			  { .pitch = 69, .period = 16.0f, .offset = 2 * 1.5f, .duration = 0.5f, .channel = 1 },
			  { .pitch = 72, .period = 16.0f, .offset = 2 * 1.5f, .duration = 0.5f, .channel = 1 },
			} )
		mEngine.addNote( note );

	for ( const PeriodicCC &cc : initializer_list<PeriodicCC>{
			  { .number = 32, .period = 4.0f, .offset = 0.1f, .channel = 0 },
			  { .number = 42, .period = 4.0f, .offset = 0.1f, .channel = 0 },
			} )
		mEngine.addCc( cc );

	pl_info( "Plugin Started" );
}

PLoops::~PLoops() noexcept {
	pl_info( "Done. Bye bye." );
}

AudioProcessorEditor *PLoops::createEditor() {
	pl_debug( "Create editor." );
	try {
		return new ui::PLoopsUi( *this );
	} catch ( ... ) {
		pl_error( "Could not create editor" );
		throw;
	}
}

bool PLoops::isBusesLayoutSupported( const BusesLayout &layouts ) const {
	if ( PluginHostType().isProTools() )
		return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
	return true;
}

void PLoops::prepareToPlay( double sampleRate, int samplesPerBlock ) {
	mEngine.prepare( sampleRate, samplesPerBlock );
}

void PLoops::releaseResources() {
}

void PLoops::reset() {
	mEngine.reset();
}

void PLoops::processBlock( AudioBuffer<float> &buffer, MidiBuffer &midi ) {
	buffer.clear();
	midi.clear();
	mEngine.process( midi, buffer.getNumSamples(), getPlayHead(), wrapperType );
}

void PLoops::processBlock( AudioBuffer<double> &, MidiBuffer & ) {
	pl_debug( "TODO: Double precision" );
}

void PLoops::getStateInformation( MemoryBlock &destData ) {
	const auto xml = mEngine.captureState().toXml();
	copyXmlToBinary( xml, destData );
	pl_debug( "getStateInformation: saved " + to_string( mEngine.getNotes().size() ) + " notes, "
	          + to_string( mEngine.getCCs().size() ) + " CCs" );
}

void PLoops::setStateInformation( const void *data, int sizeInBytes ) {
	const auto xml = getXmlFromBinary( data, sizeInBytes );
	if ( !xml ) {
		pl_error( "setStateInformation: invalid or missing state XML" );
		return;
	}
	const auto state = PluginState::fromXml( *xml );
	if ( !state ) {
		pl_error( "setStateInformation: unrecognised state format" );
		return;
	}
	mEngine.applyState( *state );
	pl_debug( "setStateInformation: loaded " + to_string( mEngine.getNotes().size() ) + " notes, "
	          + to_string( mEngine.getCCs().size() ) + " CCs" );
}

File PLoops::logFile() {
	return utils::getPlDataFolder( true ).getChildFile( string{ utils::PLUGIN_NAME } + utils::LOG_EXTENSION );
}
