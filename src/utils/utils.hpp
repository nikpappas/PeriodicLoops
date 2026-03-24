#ifndef PLOP_SRC_UTILS_UTILS_HPP
#define PLOP_SRC_UTILS_UTILS_HPP

#include <string_view>

#include "constants.hpp"

#include <juce_core/juce_core.h>

namespace plop::utils {

	inline ::juce::File get_app_data_folder() {

		return ( PL_OS_PLATFORM == os_platform::PL_OS_WINDOWS
		           ? ::juce::File::getSpecialLocation( ::juce::File::userApplicationDataDirectory )
		           : ::juce::File::getSpecialLocation( ::juce::File::userApplicationDataDirectory ).getChildFile( SETTINGS_MACOS_FOLDER ) );
	}

	inline ::juce::File create_child_dir( const ::juce::File &parent, ::juce::StringRef dir_name ) {
		auto child = parent.getChildFile( dir_name );
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
	inline ::juce::String to_juce_string( const std::string_view &prm_string_view ) {
		return { prm_string_view.data(), prm_string_view.length() };
	}

	inline ::juce::File create_pl_data_folder() {
		return create_child_dir( get_app_data_folder(), NLOOPS_FOLDER );
	}

	inline ::juce::File get_pl_data_folder( bool create_if_not_present ) {
		if ( create_if_not_present ) {
			return create_pl_data_folder();
		}

		return get_app_data_folder().getChildFile( NLOOPS_FOLDER );
	}

	constexpr float samplesToSeconds( const int &sampleRate, const int &samples ) {
		return static_cast<float>( samples ) / static_cast<float>( sampleRate );
	}

	constexpr float beatsToSeconds( const float &bpm, const float &beats ) {
		return beats / ( bpm * 60 );
	}

	constexpr float secondsToBeats( const float &bpm, const float &seconds ) {
		return ( bpm * 60 ) / seconds;
	}

	constexpr float samplesToBeats( const float &bpm, const float &sampleRate, const int &samples ) {
		const auto seconds = samplesToSeconds( sampleRate, samples );
		return secondsToBeats( bpm, seconds );
	}

} // namespace plop::utils

#endif // PLOP_SRC_UTILS_UTILS_HPP
