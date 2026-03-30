#include "periodic_loops.hpp"

#include "logging/logging.hpp"
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

using ::std::initializer_list;
using ::std::string;
using ::std::to_string;

using namespace ::plop::p_loops;

p_loops::p_loops() : AudioProcessor( p_loops::get_midifx_bus_layout() ) {

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

p_loops::~p_loops() noexcept {
   pl_info( "Done. Bye bye." );
}

AudioProcessorEditor *p_loops::createEditor() {
   pl_debug( "Create editor." );
   try {
      return new ui::p_loops_ui( *this );
   } catch ( ... ) {
      pl_error( "Could not create editor" );
      throw;
   }
}

bool p_loops::isBusesLayoutSupported( const BusesLayout &layouts ) const {
   if ( PluginHostType().isProTools() )
      return layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
   return true;
}

void p_loops::prepareToPlay( double sampleRate, int samplesPerBlock ) {
   mEngine.prepare( sampleRate, samplesPerBlock );
}

void p_loops::releaseResources() {
}

void p_loops::reset() {
   mEngine.reset();
}

void p_loops::processBlock( AudioBuffer<float> &buffer, MidiBuffer &midi ) {
   buffer.clear();
   midi.clear();
   mEngine.process( midi, buffer.getNumSamples(), getPlayHead(), wrapperType );
}

void p_loops::processBlock( AudioBuffer<double> &, MidiBuffer & ) {
   pl_debug( "TODO: Double precision" );
}

void p_loops::getStateInformation( MemoryBlock &destData ) {
   juce::XmlElement root( "PeriodicLoopState" );

   auto *notesEl = root.createNewChildElement( "Notes" );
   for ( const auto &note : mEngine.getNotes() ) {
      auto *el = notesEl->createNewChildElement( "Note" );
      el->setAttribute( "pitch", note.pitch );
      el->setAttribute( "period", note.period );
      el->setAttribute( "offset", note.offset );
      el->setAttribute( "duration", note.duration );
      el->setAttribute( "channel", note.channel );
   }

   root.setAttribute( "mode", mEngine.getMode() );
   root.setAttribute( "silicaMode", mEngine.getSilicaMode() );
   root.setAttribute( "silicaPeriod", mEngine.getSilicaPeriod() );
   root.setAttribute( "scaleRoot", mEngine.getScaleRoot() );
   root.setAttribute( "scaleType", mEngine.getScaleType() );

   auto *ccsEl = root.createNewChildElement( "CCs" );
   for ( const auto &cc : mEngine.getCCs() ) {
      auto *el = ccsEl->createNewChildElement( "CC" );
      el->setAttribute( "number", cc.number );
      el->setAttribute( "period", cc.period );
      el->setAttribute( "offset", cc.offset );
      el->setAttribute( "channel", cc.channel );
   }

   copyXmlToBinary( root, destData );
   pl_debug( "getStateInformation: saved " + to_string( mEngine.getNotes().size() ) + " notes, "
             + to_string( mEngine.getCCs().size() ) + " CCs" );
}

void p_loops::setStateInformation( const void *data, int sizeInBytes ) {
   auto xml = getXmlFromBinary( data, sizeInBytes );
   if ( !xml || xml->getTagName() != "PeriodicLoopState" ) {
      pl_error( "setStateInformation: invalid or missing state XML" );
      return;
   }

   const auto *notesEl = xml->getChildByName( "Notes" );
   if ( !notesEl ) {
      pl_error( "setStateInformation: missing Notes element" );
      return;
   }

   while ( !mEngine.getNotes().empty() )
      mEngine.removeNote( 0 );

   for ( const auto *el : notesEl->getChildIterator() ) {
      mEngine.addNote( PeriodicNote{
        .pitch    = el->getIntAttribute( "pitch", 60 ),
        .period   = static_cast<float>( el->getDoubleAttribute( "period", 1.0 ) ),
        .offset   = static_cast<float>( el->getDoubleAttribute( "offset", 0.0 ) ),
        .duration = static_cast<float>( el->getDoubleAttribute( "duration", 0.5 ) ),
        .channel  = el->getIntAttribute( "channel", 0 ),
      } );
   }

   mEngine.setMode( xml->getIntAttribute( "mode", 1 ) );
   mEngine.setSilicaMode( xml->getBoolAttribute( "silicaMode", false ) );
   mEngine.setSilicaPeriod( static_cast<float>( xml->getDoubleAttribute( "silicaPeriod", 4.0 ) ) );
   mEngine.setScaleRoot( xml->getIntAttribute( "scaleRoot", 0 ) );
   mEngine.setScaleType( xml->getIntAttribute( "scaleType", 1 ) );

   while ( !mEngine.getCCs().empty() )
      mEngine.removeCc( 0 );

   if ( const auto *ccsEl = xml->getChildByName( "CCs" ) ) {
      for ( const auto *el : ccsEl->getChildIterator() ) {
         mEngine.addCc( PeriodicCC{
           .number  = el->getIntAttribute( "number", 1 ),
           .period  = static_cast<float>( el->getDoubleAttribute( "period", 1.0 ) ),
           .offset  = static_cast<float>( el->getDoubleAttribute( "offset", 0.0 ) ),
           .channel = el->getIntAttribute( "channel", 0 ),
         } );
      }
   }

   pl_debug( "setStateInformation: loaded " + to_string( mEngine.getNotes().size() ) + " notes, "
             + to_string( mEngine.getCCs().size() ) + " CCs" );
}

File p_loops::log_file() {
   return utils::get_pl_data_folder( true ).getChildFile( string{ utils::PLUGIN_NAME } + utils::LOG_EXTENSION );
}
