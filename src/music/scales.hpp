#ifndef PLOP_SRC_MUSIC_SCALES_HPP
#define PLOP_SRC_MUSIC_SCALES_HPP

#include <array>
#include <cstddef>

namespace plop::music {

	struct ScaleDef {
		const char          *name;
		std::array<bool, 12> pitchClasses; // true = pitch class is in the scale
	};

	inline const std::array<ScaleDef, 10> SCALES = { {
	  { "Chromatic", { { true, true, true, true, true, true, true, true, true, true, true, true } } },
	  { "Major", { { true, false, true, false, true, true, false, true, false, true, false, true } } },
	  { "Minor", { { true, false, true, true, false, true, false, true, true, false, true, false } } },
	  { "Penta Maj", { { true, false, true, false, true, false, false, true, false, true, false, false } } },
	  { "Penta Min", { { true, false, false, true, false, true, false, true, false, false, true, false } } },
	  { "Blues", { { true, false, false, true, false, true, true, true, false, false, true, false } } },
	  { "Dorian", { { true, false, true, true, false, true, false, true, false, true, true, false } } },
	  { "Mixolydian", { { true, false, true, false, true, true, false, true, false, true, true, false } } },
	  { "Harm Minor", { { true, false, true, true, false, true, false, true, true, false, false, true } } },
	  { "Whole Tone", { { true, false, true, false, true, false, true, false, true, false, true, false } } },
	} };

	inline constexpr const char *NOTE_NAMES[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

	inline bool isInScale( int pitch, int root, const std::array<bool, 12> &pc ) {
		return pc[ static_cast<size_t>( ( ( pitch - root ) % 12 + 12 ) % 12 ) ];
	}

	inline int snapToScale( int pitch, int root, const std::array<bool, 12> &pc ) {
		if ( isInScale( pitch, root, pc ) )
			return pitch;
		for ( int d = 1; d <= 6; ++d ) {
			if ( pitch + d <= 127 && isInScale( pitch + d, root, pc ) )
				return pitch + d;
			if ( pitch - d >= 0 && isInScale( pitch - d, root, pc ) )
				return pitch - d;
		}
		return pitch;
	}

	inline int stepInScale( int pitch, int direction, int root, const std::array<bool, 12> &pc ) {
		int p = pitch + ( direction > 0 ? 1 : -1 );
		while ( p >= 0 && p <= 127 ) {
			if ( isInScale( p, root, pc ) )
				return p;
			p += ( direction > 0 ? 1 : -1 );
		}
		return pitch;
	}

} // namespace plop::music

#endif // PLOP_SRC_MUSIC_SCALES_HPP
