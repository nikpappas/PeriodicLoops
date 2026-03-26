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

struct PeriodicCC {
	/// @brief  [0,127]
	int number;
	/// @brief  in beats
	float period;
	/// @brief  in beats
	float offset;
	/// @brief  [0,15]
	int channel;
};

#endif // PLOP_SRC_MUSIC_MIDI_HPP
