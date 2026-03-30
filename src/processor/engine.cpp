#include "engine.hpp"

#include <algorithm>
#include <cmath>

using ::juce::MidiBuffer;
using ::juce::MidiMessage;

using ::std::ceil;
using ::std::clamp;
using ::std::memory_order_acquire;
using ::std::memory_order_release;

namespace plop::engine {

   Engine::Engine() = default;

   void Engine::prepare( double sampleRate, int blockSize ) {
      mSampleRate = sampleRate;
      mBlockSize  = blockSize;
      mTime       = 0;
   }

   void Engine::reset() {
   }

   void Engine::process( MidiBuffer &midi,
                         int         numSamples,
                         ::juce::AudioPlayHead *playHead,
                         ::juce::AudioProcessor::WrapperType wrapperType ) {
      // Send all-notes-off when standalone pauses
      if ( mSendAllNotesOff.exchange( false, ::std::memory_order_acq_rel ) ) {
         for ( int ch = 1; ch <= 16; ++ch )
            midi.addEvent( MidiMessage::allNotesOff( ch ), 0 );
         mTime += numSamples;
         return;
      }

      const NoteBuffer &noteBuf = mNoteBuf[ mActiveNoteBuf.load( memory_order_acquire ) ];

      if ( noteBuf.count == 0 ) {
         mTime += numSamples;
         return;
      }

      // Derive blockStartBeat from host playhead when available, fall back to sample counter
      float ppqPosition = -1.0f;
      if ( playHead != nullptr ) {
         auto posInfo = playHead->getPosition();
         if ( posInfo.hasValue() ) {
            if ( posInfo->getBpm().hasValue() )
               mBpm.store( static_cast<float>( *posInfo->getBpm() ) );

            if ( !posInfo->getIsPlaying() && wrapperType != ::juce::AudioProcessor::wrapperType_Standalone )
               return;

            if ( posInfo->getIsPlaying() && posInfo->getPpqPosition().hasValue() )
               ppqPosition = static_cast<float>( *posInfo->getPpqPosition() );
         }
      }

      // Standalone pause
      if ( wrapperType == ::juce::AudioProcessor::wrapperType_Standalone
           && !mStandalonePlaying.load( memory_order_acquire ) ) {
         mTime += numSamples;
         return;
      }

      const float samplesPerBeat = static_cast<float>( mSampleRate ) * 60.0f / mBpm.load();
      const float blockStartBeat =
        ppqPosition >= 0.0f ? ppqPosition : static_cast<float>( mTime.load() ) / samplesPerBeat;
      const float blockEndBeat = blockStartBeat + static_cast<float>( numSamples ) / samplesPerBeat;

      if ( ppqPosition >= 0.0f )
         mTime.store( static_cast<int64_t>( blockStartBeat * samplesPerBeat ) );

      // NoteOff pass — must come before NoteOn so same-sample ordering is correct
      for ( int ni = 0; ni < noteBuf.count; ++ni ) {
         const auto &note = noteBuf.notes[ ni ];
         if ( note.period <= 0.0f )
            continue;

         auto n = static_cast<int>( ceil( ( blockStartBeat - note.duration - note.offset ) / note.period - 1e-6f ) );
         if ( n < 0 )
            n = 0;

         while ( true ) {
            const float offBeat = static_cast<float>( n ) * note.period + note.duration + note.offset;
            if ( offBeat >= blockEndBeat )
               break;
            if ( offBeat >= blockStartBeat ) {
               const int offset =
                 clamp( static_cast<int>( ( offBeat - blockStartBeat ) * samplesPerBeat ), 0, numSamples - 1 );
               midi.addEvent( MidiMessage::noteOff( note.channel + 1, note.pitch ), offset );
            }
            ++n;
         }
      }

      // NoteOn pass
      for ( int ni = 0; ni < noteBuf.count; ++ni ) {
         const auto &note = noteBuf.notes[ ni ];
         if ( note.period <= 0.0f )
            continue;

         auto n = static_cast<int>( ceil( ( blockStartBeat - note.offset ) / note.period - 1e-6f ) );
         if ( n < 0 )
            n = 0;

         while ( true ) {
            const float onBeat = static_cast<float>( n ) * note.period + note.offset;
            if ( onBeat >= blockEndBeat )
               break;
            const int offset =
              clamp( static_cast<int>( ( onBeat - blockStartBeat ) * samplesPerBeat ), 0, numSamples - 1 );
            midi.addEvent( MidiMessage::noteOn( note.channel + 1, note.pitch, (uint8_t) 100 ), offset );
            ++n;
         }
      }

      const int       ccSamplingStep = static_cast<int>( mSampleRate / 100.0 ); // 100 Hz max
      const CCBuffer &ccBuf          = mCcBuf[ mActiveCcBuf.load( memory_order_acquire ) ];

      if ( ccBuf.count > 0 ) {
         bool anySolo = false;
         for ( int ni = 0; ni < ccBuf.count; ++ni ) {
            if ( ccBuf.cc[ ni ].solo ) {
               anySolo = true;
               break;
            }
         }

         const int64_t timeNow = mTime.load();
         for ( int i = 0; i < numSamples; i += ccSamplingStep ) {
            for ( int ni = 0; ni < ccBuf.count; ++ni ) {
               const auto &cc = ccBuf.cc[ ni ];
               if ( cc.period <= 0.0f )
                  continue;
               if ( anySolo && !cc.solo )
                  continue;
               midi.addEvent(
                 MidiMessage::controllerEvent(
                   cc.channel + 1,
                   cc.number,
                   static_cast<int>(
                     127.0f * ( 0.5f + 0.5f * sin( 2 * 3.14f * ( timeNow + i + cc.offset * samplesPerBeat )
                                                    / ( cc.period * samplesPerBeat ) ) ) ) ),
                 timeNow + i );
            }
         }
      }

      mTime += numSamples;
   }

   // ---- Note management -------------------------------------------------------

   void Engine::addNote( const PeriodicNote &note ) {
      swapNoteBuffer( [ & ]( NoteBuffer &buf ) {
         if ( buf.count < MAX_NOTES )
            buf.notes[ buf.count++ ] = note;
      } );
      mUiNotes.push_back( note );
      if ( mSilicaMode )
         redistributeSilicaOffsets();
   }

   void Engine::removeNote( int index ) {
      if ( index < 0 || index >= static_cast<int>( mUiNotes.size() ) )
         return;
      swapNoteBuffer( [ index ]( NoteBuffer &buf ) {
         if ( index < buf.count ) {
            for ( int i = index; i < buf.count - 1; ++i )
               buf.notes[ i ] = buf.notes[ i + 1 ];
            --buf.count;
         }
      } );
      mUiNotes.erase( mUiNotes.begin() + index );
      if ( mSilicaMode )
         redistributeSilicaOffsets();
   }

   void Engine::updateNote( int index, const PeriodicNote &note ) {
      if ( index < 0 || index >= static_cast<int>( mUiNotes.size() ) )
         return;
      swapNoteBuffer( [ index, &note ]( NoteBuffer &buf ) {
         if ( index < buf.count )
            buf.notes[ index ] = note;
      } );
      mUiNotes[ index ] = note;
   }

   // ---- CC management ---------------------------------------------------------

   void Engine::addCc( const PeriodicCC &cc ) {
      swapCCBuffer( [ & ]( CCBuffer &buf ) {
         if ( buf.count < MAX_NOTES )
            buf.cc[ buf.count++ ] = cc;
      } );
      mUiCcs.push_back( cc );
   }

   void Engine::removeCc( int index ) {
      if ( index < 0 || index >= static_cast<int>( mUiCcs.size() ) )
         return;
      swapCCBuffer( [ index ]( CCBuffer &buf ) {
         if ( index < buf.count ) {
            for ( int i = index; i < buf.count - 1; ++i )
               buf.cc[ i ] = buf.cc[ i + 1 ];
            --buf.count;
         }
      } );
      mUiCcs.erase( mUiCcs.begin() + index );
   }

   void Engine::updateCc( int index, const PeriodicCC &cc ) {
      if ( index < 0 || index >= static_cast<int>( mUiCcs.size() ) )
         return;
      swapCCBuffer( [ index, &cc ]( CCBuffer &buf ) {
         if ( index < buf.count )
            buf.cc[ index ] = cc;
      } );
      mUiCcs[ index ] = cc;
   }

   // ---- Silica mode -----------------------------------------------------------

   void Engine::setSilicaMode( bool enabled ) {
      mSilicaMode = enabled;
      if ( enabled )
         redistributeSilicaOffsets();
   }

   void Engine::setSilicaPeriod( float period ) {
      mSilicaPeriod = ::juce::jmax( 0.01f, period );
      if ( mSilicaMode )
         redistributeSilicaOffsets();
   }

   void Engine::redistributeSilicaOffsets() {
      const int n = static_cast<int>( mUiNotes.size() );
      if ( n == 0 )
         return;

      for ( int i = 0; i < n; ++i ) {
         mUiNotes[ i ].period = mSilicaPeriod;
         mUiNotes[ i ].offset = static_cast<float>( i ) * mSilicaPeriod / static_cast<float>( n );
      }

      swapNoteBuffer( [ this, n ]( NoteBuffer &buf ) {
         buf.count = n;
         for ( int i = 0; i < n; ++i )
            buf.notes[ i ] = mUiNotes[ i ];
      } );
   }

   // ---- Playback state --------------------------------------------------------

   void Engine::setStandalonePlaying( bool playing ) {
      if ( !playing && mStandalonePlaying.load( ::std::memory_order_relaxed ) )
         mSendAllNotesOff.store( true, memory_order_release );
      mStandalonePlaying.store( playing, memory_order_release );
   }

} // namespace plop::engine
