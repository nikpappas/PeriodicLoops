#ifndef PLOP_SRC_UTILS_UTILS_HPP
#define PLOP_SRC_UTILS_UTILS_HPP

#include <string_view>

#include "constants.hpp"

#include <juce_core/juce_core.h>

namespace plop::utils {

	inline ::juce::File getAppDataFolder() {

		return ( PL_OS_PLATFORM == OsPlatform::PL_OS_WINDOWS
		           ? ::juce::File::getSpecialLocation( ::juce::File::userApplicationDataDirectory )
		           : ::juce::File::getSpecialLocation( ::juce::File::userApplicationDataDirectory ).getChildFile( SETTINGS_MACOS_FOLDER ) );
	}

	inline ::juce::File createChildDir( const ::juce::File &parent, const ::juce::StringRef &dirName ) {
		auto child = parent.getChildFile( dirName );
		if ( !child.isDirectory() ) {
			child.createDirectory();
		}

		return child;
	}

	/// \brief Make a Juce String from the specified std::string_view
	///
	/// Since string_view knows its own length we can pass that length to the Juce String constructor
	/// so it won't ever have to either scan the data multiple times or allocate multiple times
	/// (and it's possible to include a null in the data)
	inline ::juce::String toJuceString( const std::string_view &sv ) {
		return { sv.data(), sv.length() };
	}

	inline ::juce::File createPlDataFolder() {
		return createChildDir( getAppDataFolder(), NLOOPS_FOLDER );
	}

	inline ::juce::File getPlDataFolder( bool createIfNotPresent ) {
		if ( createIfNotPresent ) {
			return createPlDataFolder();
		}

		return getAppDataFolder().getChildFile( NLOOPS_FOLDER );
	}

	constexpr float samplesToSeconds( const int &sampleRate, const int &samples ) {
		return static_cast<float>( samples ) / static_cast<float>( sampleRate );
	}

	constexpr float beatsToSeconds( const float &bpm, const float &beats ) {
		return beats / ( bpm * 60 );
	}

	constexpr float beatsToSamples( const float &bpm, const float &beats, const float &sr ) {
		return beatsToSeconds( bpm, beats ) * sr;
	}

	constexpr float secondsToBeats( const float &bpm, const float &seconds ) {
		return ( bpm * 60 ) / seconds;
	}

	constexpr float samplesToBeats( const float &bpm, const float &sampleRate, const int &samples ) {
		const auto seconds = samplesToSeconds( static_cast<int>( sampleRate ), samples );
		return secondsToBeats( bpm, seconds );
	}

} // namespace plop::utils

#endif // PLOP_SRC_UTILS_UTILS_HPP
