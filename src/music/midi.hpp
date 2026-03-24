#ifndef PLOP_SRC_MUSIC_MIDI_HPP
#define PLOP_SRC_MUSIC_MIDI_HPP

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

inline bool operator<( const PeriodicNote &a, const PeriodicNote &b ) {
	return a.pitch < b.pitch;
}

inline bool operator==( const PeriodicNote &a, const PeriodicNote &b ) {
	return a.pitch == b.pitch && a.period == b.period && a.duration == b.duration;
}

#endif // PLOP_SRC_MUSIC_MIDI_HPP
