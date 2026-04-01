#include "plugin_state.hpp"

#include "music/patterns.hpp"

namespace plop {

	constexpr int fromPluginMode( const PluginMode &mode ) {
		switch ( mode ) {
			case PluginMode::Pro:
				return 0;
			case PluginMode::Melody:
				return 1;
			case PluginMode::Drums:
				return 2;
			case PluginMode::Silica:
				return 3;
		}
		return 1;
	}

	constexpr PluginMode fromInt( const int &modeInt ) {
		switch ( modeInt ) {
			case 0:
				return PluginMode::Pro;
			case 1:
				return PluginMode::Melody;
			case 2:
				return PluginMode::Drums;
			case 3:
				return PluginMode::Silica;
			case 4:
				return PluginMode::Melody; // legacy Scale mode -> Melody
			default:
				return PluginMode::Melody;
		}
	}

	::juce::XmlElement PluginState::toXml() const {
		::juce::XmlElement root( "PeriodicLoopState" );

		// Legacy flat notes (for backward compat / Pro mode)
		auto *notesEl = root.createNewChildElement( "Notes" );
		for ( const auto &note : notes ) {
			auto *el = notesEl->createNewChildElement( "Note" );
			el->setAttribute( "pitch", note.pitch );
			el->setAttribute( "period", note.period );
			el->setAttribute( "offset", note.offset );
			el->setAttribute( "duration", note.duration );
			el->setAttribute( "channel", note.channel );
		}

		root.setAttribute( "mode", fromPluginMode( mode ) );
		root.setAttribute( "silicaPeriod", silicaPeriod );
		root.setAttribute( "scaleRoot", scaleRoot );
		root.setAttribute( "scaleType", scaleType );

		auto *ccsEl = root.createNewChildElement( "CCs" );
		for ( const auto &cc : ccs ) {
			auto *el = ccsEl->createNewChildElement( "CC" );
			el->setAttribute( "number", cc.number );
			el->setAttribute( "period", cc.period );
			el->setAttribute( "offset", cc.offset );
			el->setAttribute( "channel", cc.channel );
			el->setAttribute( "shape", static_cast<int>( cc.shape ) );
		}

		// Groups
		auto *groupsEl = root.createNewChildElement( "Groups" );
		for ( const auto &group : groups ) {
			auto *gEl = groupsEl->createNewChildElement( "Group" );
			gEl->setAttribute( "colour", static_cast<int>( group.colour.getARGB() ) );
			gEl->setAttribute( "period", group.period );
			gEl->setAttribute( "channel", group.channel );
			gEl->setAttribute( "muted", group.muted );
			gEl->setAttribute( "solo", group.solo );
			gEl->setAttribute( "expanded", group.expanded );
			gEl->setAttribute( "rootPitch", group.rootPitch );
			gEl->setAttribute( "noteCount", group.noteCount );
			gEl->setAttribute( "pattern", music::patternFunctionToInt( group.pattern ) );
			gEl->setAttribute( "groupMode", fromPluginMode( group.mode ) );

			for ( const auto &voice : group.voices ) {
				auto *vEl = gEl->createNewChildElement( "Voice" );
				vEl->setAttribute( "pitch", voice.pitch );
				vEl->setAttribute( "period", voice.period );
				vEl->setAttribute( "offset", voice.offset );
				vEl->setAttribute( "duration", voice.duration );
				vEl->setAttribute( "channel", voice.channel );
			}
		}

		return root;
	}

	::std::optional<PluginState> PluginState::fromXml( const ::juce::XmlElement &xml ) {
		if ( xml.getTagName() != "PeriodicLoopState" )
			return ::std::nullopt;

		const auto *notesEl = xml.getChildByName( "Notes" );
		if ( !notesEl )
			return ::std::nullopt;

		PluginState state;

		for ( const auto *el : notesEl->getChildIterator() ) {
			state.notes.push_back( PeriodicNote{
			  .pitch    = el->getIntAttribute( "pitch", 60 ),
			  .period   = static_cast<float>( el->getDoubleAttribute( "period", 1.0 ) ),
			  .offset   = static_cast<float>( el->getDoubleAttribute( "offset", 0.0 ) ),
			  .duration = static_cast<float>( el->getDoubleAttribute( "duration", 0.5 ) ),
			  .channel  = el->getIntAttribute( "channel", 0 ),
			} );
		}

		state.mode         = fromInt( xml.getIntAttribute( "mode", 1 ) );
		state.silicaPeriod = static_cast<float>( xml.getDoubleAttribute( "silicaPeriod", 4.0 ) );
		state.scaleRoot    = xml.getIntAttribute( "scaleRoot", 0 );
		state.scaleType    = xml.getIntAttribute( "scaleType", 1 );

		if ( const auto *ccsEl = xml.getChildByName( "CCs" ) ) {
			for ( const auto *el : ccsEl->getChildIterator() ) {
				state.ccs.push_back( PeriodicCC{
				  .number  = el->getIntAttribute( "number", 1 ),
				  .period  = static_cast<float>( el->getDoubleAttribute( "period", 1.0 ) ),
				  .offset  = static_cast<float>( el->getDoubleAttribute( "offset", 0.0 ) ),
				  .channel = el->getIntAttribute( "channel", 0 ),
				  .shape   = static_cast<WaveShape>( el->getIntAttribute( "shape", 0 ) ),
				} );
			}
		}

		if ( const auto *groupsEl = xml.getChildByName( "Groups" ) ) {
			for ( const auto *gEl : groupsEl->getChildIterator() ) {
				NoteGroup group;
				group.colour    = ::juce::Colour( static_cast<uint32_t>( gEl->getIntAttribute( "colour", static_cast<int>( 0xff4fc3f7 ) ) ) );
				group.period    = static_cast<float>( gEl->getDoubleAttribute( "period", 4.0 ) );
				group.channel   = gEl->getIntAttribute( "channel", 0 );
				group.muted     = gEl->getBoolAttribute( "muted", false );
				group.solo      = gEl->getBoolAttribute( "solo", false );
				group.expanded  = gEl->getBoolAttribute( "expanded", true );
				group.rootPitch = gEl->getIntAttribute( "rootPitch", 60 );
				group.noteCount = gEl->getIntAttribute( "noteCount", 1 );
				group.pattern   = music::patternFunctionFromInt( gEl->getIntAttribute( "pattern", 0 ) );
				// Default to the plugin-level mode so old saves load with the correct group type
				group.mode      = fromInt( gEl->getIntAttribute( "groupMode", fromPluginMode( state.mode ) ) );

				for ( const auto *vEl : gEl->getChildIterator() ) {
					group.voices.push_back( PeriodicNote{
					  .pitch    = vEl->getIntAttribute( "pitch", 60 ),
					  .period   = static_cast<float>( vEl->getDoubleAttribute( "period", 4.0 ) ),
					  .offset   = static_cast<float>( vEl->getDoubleAttribute( "offset", 0.0 ) ),
					  .duration = static_cast<float>( vEl->getDoubleAttribute( "duration", 0.5 ) ),
					  .channel  = vEl->getIntAttribute( "channel", 0 ),
					} );
				}

				state.groups.push_back( std::move( group ) );
			}
		}

		return state;
	}

} // namespace plop
