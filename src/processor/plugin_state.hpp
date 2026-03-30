#ifndef PLOP_SRC_PROCESSOR_PLUGIN_STATE_HPP
#define PLOP_SRC_PROCESSOR_PLUGIN_STATE_HPP

#include <optional>
#include <vector>

#include <juce_core/juce_core.h>

#include "music/midi.hpp"

namespace plop {

	enum class PluginMode { pro, melody, drums, silica, scale };

	struct PluginState {
		::std::vector<PeriodicNote> notes;
		::std::vector<PeriodicCC>   ccs;
		PluginMode                  mode         = PluginMode::melody;
		bool                        silicaMode   = false;
		float                       silicaPeriod = 4.0f;
		int                         scaleRoot    = 0; // 0 = C
		int                         scaleType    = 1; // index into music::SCALES (1 = Major)

		::juce::XmlElement toXml() const;

		/// Returns nullopt if xml is missing or has an unrecognised tag.
		static ::std::optional<PluginState> fromXml( const ::juce::XmlElement &xml );
	};

} // namespace plop

#endif // PLOP_SRC_PROCESSOR_PLUGIN_STATE_HPP
