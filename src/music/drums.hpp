#ifndef PLOP_SRC_MUSIC_DRUMS_HPP
#define PLOP_SRC_MUSIC_DRUMS_HPP

#include <array>
#include <cstddef>

namespace plop::music {

	struct GmDrum {
		int         note;
		const char *name; // max 8 chars — fits the 65 px pitch column
	};

	// GM standard percussion map (MIDI notes 35–81, channel 10 / index 9)
	static constexpr std::array<GmDrum, 47> kGmDrums = { {
	  { 35, "Ac BsDrm" }, // Acoustic Bass Drum
	  { 36, "Bass Drm" }, // Bass Drum 1
	  { 37, "SideStck" }, // Side Stick
	  { 38, "Snare" },    // Acoustic Snare
	  { 39, "Hnd Clap" }, // Hand Clap
	  { 40, "El Snare" }, // Electric Snare
	  { 41, "LoFlrTom" }, // Low Floor Tom
	  { 42, "HH Close" }, // Closed Hi-Hat
	  { 43, "HiFlrTom" }, // High Floor Tom
	  { 44, "HH Pedal" }, // Pedal Hi-Hat
	  { 45, "Low Tom" },  // Low Tom
	  { 46, "HH Open" },  // Open Hi-Hat
	  { 47, "LowMdTom" }, // Low-Mid Tom
	  { 48, "HiMidTom" }, // Hi-Mid Tom
	  { 49, "Crash 1" },  // Crash Cymbal 1
	  { 50, "High Tom" }, // High Tom
	  { 51, "Ride 1" },   // Ride Cymbal 1
	  { 52, "ChnCymbl" }, // Chinese Cymbal
	  { 53, "RideBell" }, // Ride Bell
	  { 54, "Tambrine" }, // Tambourine
	  { 55, "Splash" },   // Splash Cymbal
	  { 56, "Cowbell" },  // Cowbell
	  { 57, "Crash 2" },  // Crash Cymbal 2
	  { 58, "Vibslap" },  // Vibraslap
	  { 59, "Ride 2" },   // Ride Cymbal 2
	  { 60, "Hi Bongo" }, // Hi Bongo
	  { 61, "Lo Bongo" }, // Low Bongo
	  { 62, "MtHiCnga" }, // Mute Hi Conga
	  { 63, "OpHiCnga" }, // Open Hi Conga
	  { 64, "Lo Conga" }, // Low Conga
	  { 65, "HiTimbal" }, // High Timbale
	  { 66, "LoTimbal" }, // Low Timbale
	  { 67, "Hi Agogo" }, // High Agogo
	  { 68, "Lo Agogo" }, // Low Agogo
	  { 69, "Cabasa" },   // Cabasa
	  { 70, "Maracas" },  // Maracas
	  { 71, "ShWhistl" }, // Short Whistle
	  { 72, "LgWhistl" }, // Long Whistle
	  { 73, "ShGuiro" },  // Short Guiro
	  { 74, "LgGuiro" },  // Long Guiro
	  { 75, "Claves" },   // Claves
	  { 76, "HiWdBlk" },  // Hi Wood Block
	  { 77, "LoWdBlk" },  // Low Wood Block
	  { 78, "MtCuica" },  // Mute Cuica
	  { 79, "OpCuica" },  // Open Cuica
	  { 80, "MtTrngl" },  // Mute Triangle
	  { 81, "OpTrngl" },  // Open Triangle
	} };

	/// Returns the name for a standard GM drum note, or nullptr if not in the map.
	inline const char *gmDrumName( int note ) {
		for ( const auto &d : kGmDrums )
			if ( d.note == note )
				return d.name;
		return nullptr;
	}

	/// Returns the index in kGmDrums for the given note, snapping to the nearest
	/// entry at or above the note if there is no exact match.
	inline int gmDrumIndexForNote( int note ) {
		for ( int i = 0; i < static_cast<int>( kGmDrums.size() ); ++i )
			if ( kGmDrums[ i ].note >= note )
				return i;
		return static_cast<int>( kGmDrums.size() ) - 1;
	}

	/// Returns the drum note number at the given index, clamped to valid range.
	inline int gmDrumNoteAtIndex( int idx ) {
		if ( idx < 0 )
			idx = 0;
		if ( idx >= static_cast<int>( kGmDrums.size() ) )
			idx = static_cast<int>( kGmDrums.size() ) - 1;
		return kGmDrums[ idx ].note;
	}

} // namespace plop::music

#endif // PLOP_SRC_MUSIC_DRUMS_HPP
