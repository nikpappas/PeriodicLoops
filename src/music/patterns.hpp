#ifndef PLOP_SRC_MUSIC_PATTERNS_HPP
#define PLOP_SRC_MUSIC_PATTERNS_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "music/midi.hpp"
#include "music/scales.hpp"

namespace plop::music {

   /// Expand a NoteGroup into concrete PeriodicNote voices based on the pattern function.
   /// Used by Silica and Melody modes where voices are auto-generated.
   inline ::std::vector<PeriodicNote> generateVoices( const NoteGroup              &group,
                                                      int                           scaleRoot,
                                                      const ::std::array<bool, 12> &scalePc ) {
      ::std::vector<PeriodicNote> out;
      const int                   n = ::std::max( 1, group.noteCount );
      out.reserve( static_cast<size_t>( n ) );

      auto makePitches = [ & ]() -> ::std::vector<int> {
         ::std::vector<int> pitches;
         pitches.reserve( static_cast<size_t>( n ) );
         switch ( group.pattern ) {
            case PatternFunction::Single: {
               pitches.push_back( group.rootPitch );
               break;
            }

            case PatternFunction::Repeat:
            case PatternFunction::Tremolo:
            case PatternFunction::Roll: {
               for ( int i = 0; i < n; ++i )
                  pitches.push_back( group.rootPitch );
               break;
            }

            case PatternFunction::ArpUp: {
               int p = group.rootPitch;
               for ( int i = 0; i < n; ++i ) {
                  pitches.push_back( p );
                  if ( i < n - 1 )
                     p = stepInScale( p, 1, scaleRoot, scalePc );
               }
               break;
            }

            case PatternFunction::ArpDown: {
               int p = group.rootPitch;
               for ( int i = 0; i < n; ++i ) {
                  pitches.push_back( p );
                  if ( i < n - 1 )
                     p = stepInScale( p, -1, scaleRoot, scalePc );
               }
               break;
            }

            case PatternFunction::Bounce: {
               int  p   = group.rootPitch;
               bool up  = true;
               for ( int i = 0; i < n; ++i ) {
                  pitches.push_back( p );
                  if ( i < n - 1 ) {
                     int next = stepInScale( p, up ? 1 : -1, scaleRoot, scalePc );
                     if ( next == p )
                        up = !up;
                     p = stepInScale( p, up ? 1 : -1, scaleRoot, scalePc );
                  }
               }
               break;
            }

            case PatternFunction::Trill: {
               const int upper = stepInScale( group.rootPitch, 1, scaleRoot, scalePc );
               for ( int i = 0; i < n; ++i )
                  pitches.push_back( i % 2 == 0 ? group.rootPitch : upper );
               break;
            }

            case PatternFunction::Triplet: {
               for ( int i = 0; i < n; ++i )
                  pitches.push_back( group.rootPitch );
               break;
            }

            case PatternFunction::Mordent: {
               const int upper = stepInScale( group.rootPitch, 1, scaleRoot, scalePc );
               const int seq[] = { group.rootPitch, upper, group.rootPitch };
               for ( int i = 0; i < n; ++i )
                  pitches.push_back( seq[ i % 3 ] );
               break;
            }

            case PatternFunction::Turn: {
               const int upper = stepInScale( group.rootPitch, 1, scaleRoot, scalePc );
               const int lower = stepInScale( group.rootPitch, -1, scaleRoot, scalePc );
               const int seq[] = { group.rootPitch, upper, group.rootPitch, lower, group.rootPitch };
               for ( int i = 0; i < n; ++i )
                  pitches.push_back( seq[ i % 5 ] );
               break;
            }

            case PatternFunction::Flam: {
               for ( int i = 0; i < n; ++i )
                  pitches.push_back( group.rootPitch );
               break;
            }

            case PatternFunction::Drag: {
               for ( int i = 0; i < n; ++i )
                  pitches.push_back( group.rootPitch );
               break;
            }

            case PatternFunction::Paradiddle: {
               // Needs at least 2 pitches in the group's existing voices
               const int p1 = group.rootPitch;
               const int p2 = group.voices.size() >= 2 ? group.voices[ 1 ].pitch
                                                        : stepInScale( group.rootPitch, 1, scaleRoot, scalePc );
               // RLRR LRLL pattern
               const int seq[] = { p1, p2, p1, p1, p2, p1, p2, p2 };
               for ( int i = 0; i < n; ++i )
                  pitches.push_back( seq[ i % 8 ] );
               break;
            }
         }
         return pitches;
      };

      auto makeOffsets = [ & ]() -> ::std::vector<float> {
         ::std::vector<float> offsets;
         offsets.reserve( static_cast<size_t>( n ) );

         switch ( group.pattern ) {
            case PatternFunction::Flam: {
               // Two rapid hits: grace note very close before main
               const float flamGap = ::std::min( 0.05f, group.period * 0.05f );
               for ( int i = 0; i < n; ++i ) {
                  const int   pair   = i / 2;
                  const float base   = static_cast<float>( pair ) * group.period / ::std::max( 1.0f, static_cast<float>( ( n + 1 ) / 2 ) );
                  offsets.push_back( i % 2 == 0 ? base : base + flamGap );
               }
               break;
            }
            case PatternFunction::Drag: {
               // Three rapid hits: two grace notes + main
               const float dragGap = ::std::min( 0.04f, group.period * 0.04f );
               for ( int i = 0; i < n; ++i ) {
                  const int   triplet = i / 3;
                  const float base    = static_cast<float>( triplet ) * group.period / ::std::max( 1.0f, static_cast<float>( ( n + 2 ) / 3 ) );
                  offsets.push_back( base + static_cast<float>( i % 3 ) * dragGap );
               }
               break;
            }
            case PatternFunction::Tremolo: {
               // Higher density: subdivide more finely
               for ( int i = 0; i < n; ++i )
                  offsets.push_back( static_cast<float>( i ) * group.period / static_cast<float>( n ) );
               break;
            }
            default: {
               // Even distribution
               for ( int i = 0; i < n; ++i )
                  offsets.push_back( static_cast<float>( i ) * group.period / static_cast<float>( n ) );
               break;
            }
         }
         return offsets;
      };

      const auto pitches  = makePitches();
      const auto offsets   = makeOffsets();
      const int  count     = ::std::min( static_cast<int>( pitches.size() ), n );
      const float duration = ::std::max( 0.05f, group.period / static_cast<float>( ::std::max( 1, n * 2 ) ) );

      for ( int i = 0; i < count; ++i ) {
         out.push_back( PeriodicNote{
           .pitch    = pitches[ i ],
           .period   = group.period,
           .offset   = i < static_cast<int>( offsets.size() ) ? offsets[ i ] : 0.0f,
           .duration = duration,
           .channel  = group.channel,
         } );
      }

      return out;
   }

   /// Flatten a list of groups into a single note vector for the engine.
   /// Respects mute/solo state.
   inline ::std::vector<PeriodicNote> flattenGroups( const ::std::vector<NoteGroup> &groups ) {
      bool anySolo = false;
      for ( const auto &g : groups ) {
         if ( g.solo ) {
            anySolo = true;
            break;
         }
      }

      ::std::vector<PeriodicNote> out;
      for ( const auto &g : groups ) {
         if ( g.muted )
            continue;
         if ( anySolo && !g.solo )
            continue;
         for ( const auto &v : g.voices )
            out.push_back( v );
      }
      return out;
   }

   /// Names for PatternFunction values, for UI display.
   inline const char *patternFunctionName( PatternFunction pf ) {
      switch ( pf ) {
         case PatternFunction::Single:
            return "Single";
         case PatternFunction::Repeat:
            return "Repeat";
         case PatternFunction::ArpUp:
            return "Arp Up";
         case PatternFunction::ArpDown:
            return "Arp Down";
         case PatternFunction::Bounce:
            return "Bounce";
         case PatternFunction::Trill:
            return "Trill";
         case PatternFunction::Triplet:
            return "Triplet";
         case PatternFunction::Mordent:
            return "Mordent";
         case PatternFunction::Turn:
            return "Turn";
         case PatternFunction::Tremolo:
            return "Tremolo";
         case PatternFunction::Flam:
            return "Flam";
         case PatternFunction::Drag:
            return "Drag";
         case PatternFunction::Paradiddle:
            return "Paradiddle";
         case PatternFunction::Roll:
            return "Roll";
      }
      return "Unknown";
   }

   /// Pattern functions available in each mode.
   inline const ::std::vector<PatternFunction> &drumsPatterns() {
      static const ::std::vector<PatternFunction> v = {
         PatternFunction::Single, PatternFunction::Repeat, PatternFunction::Flam,
         PatternFunction::Drag,   PatternFunction::Paradiddle, PatternFunction::Roll,
      };
      return v;
   }

   inline const ::std::vector<PatternFunction> &silicaPatterns() {
      static const ::std::vector<PatternFunction> v = {
         PatternFunction::Single, PatternFunction::Repeat,  PatternFunction::ArpUp,
         PatternFunction::ArpDown, PatternFunction::Bounce,
      };
      return v;
   }

   inline const ::std::vector<PatternFunction> &melodyPatterns() {
      static const ::std::vector<PatternFunction> v = {
         PatternFunction::Single, PatternFunction::Trill,   PatternFunction::Triplet,
         PatternFunction::Mordent, PatternFunction::Turn,   PatternFunction::Tremolo,
      };
      return v;
   }

   inline int patternFunctionToInt( PatternFunction pf ) {
      return static_cast<int>( pf );
   }

   inline PatternFunction patternFunctionFromInt( int v ) {
      if ( v >= 0 && v <= static_cast<int>( PatternFunction::Roll ) )
         return static_cast<PatternFunction>( v );
      return PatternFunction::Single;
   }

} // namespace plop::music

#endif // PLOP_SRC_MUSIC_PATTERNS_HPP
