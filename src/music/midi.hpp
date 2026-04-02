#ifndef PLOP_SRC_MUSIC_MIDI_HPP
#define PLOP_SRC_MUSIC_MIDI_HPP

#include <cmath>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

struct PeriodicNote {
	/// @brief  [0,127]
	int pitch;
	/// @brief  in beats
	float period;
	/// @brief  in beats
	float offset;
	/// @brief  in beats
	float duration;
	/// @brief  [0,15]
	int channel;
};

namespace plop {
	enum class PluginMode { Pro, Melody, Drums, Silica };
	enum class WaveShape { Sin, Tri, Saw };

	/// Returns a normalised [0,1] wave value for the given shape and continuous phase.
	inline float evalWaveShape( WaveShape shape, float phase ) {
		switch ( shape ) {
			case WaveShape::Sin:
				return 0.5f + 0.5f * std::sin( 2.0f * 3.14159265f * phase );
			case WaveShape::Tri: {
				float t = std::fmod( phase, 1.0f );
				if ( t < 0.0f )
					t += 1.0f;
				return t < 0.5f ? 2.0f * t : 2.0f * ( 1.0f - t );
			}
			case WaveShape::Saw: {
				float t = std::fmod( phase, 1.0f );
				if ( t < 0.0f )
					t += 1.0f;
				return t;
			}
		}
		return 0.5f;
	}

	struct PeriodicCC {
		/// @brief  [0,127]
		int number;
		/// @brief  in beats
		float period;
		/// @brief  in beats
		float offset;
		/// @brief  [0,15]
		int channel;
		/// @brief  waveform shape
		WaveShape shape = WaveShape::Sin;
		/// @brief  solo flag (UI/runtime only, not serialised)
		bool solo = false;
		/// @brief  mute flag (UI/runtime only, not serialised)
		bool muted = false;
	};

	/// Pattern functions shared across modes — each mode exposes a subset.
	enum class PatternFunction {
		// Universal
		Single, ///< one note per period
		Repeat, ///< N identical notes evenly spaced

		// Melodic (Silica + Melody)
		ArpUp,   ///< step up through scale/chord
		ArpDown, ///< step down through scale/chord
		Bounce,  ///< up then down

		// Melody snippets
		Trill,   ///< alternate root <-> upper neighbour
		Triplet, ///< 3 evenly spaced repetitions
		Mordent, ///< root -> upper -> root
		Turn,    ///< root -> upper -> root -> lower -> root
		Tremolo, ///< rapid repetitions (higher density than Repeat)

		// Drums
		Flam,       ///< two rapid hits
		Drag,       ///< three rapid hits (grace + main)
		Paradiddle, ///< alternating RLRR pattern across two pitches
		Roll,       ///< rapid succession of N hits
	};

	/// A group of voices sharing a colour, period, and channel.
	struct NoteGroup {
		::juce::Colour colour{ 0xff4fc3f7 };
		float          period   = 4.0f;
		int            channel  = 0;
		bool           muted    = false;
		bool           solo     = false;
		bool           expanded = true;

		int             rootPitch = 60;
		int             noteCount = 1;
		PatternFunction pattern   = PatternFunction::Single;

		/// Per-group mode: Drums, Silica, or Melody.
		::plop::PluginMode mode = ::plop::PluginMode::Melody;

		/// In drums mode: manually edited voices.
		/// In silica/melody mode: auto-generated from (rootPitch, noteCount, pattern).
		::std::vector<PeriodicNote> voices;
	};

} // namespace plop

#endif // PLOP_SRC_MUSIC_MIDI_HPP
