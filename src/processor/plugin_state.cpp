#include "plugin_state.hpp"

namespace plop {

	constexpr int fromPluginMode( const PluginMode &mode ) {
		switch ( mode ) {
			case PluginMode::pro:
				return 0;
			case PluginMode::melody:
				return 1;
			case PluginMode::drums:
				return 2;
			case PluginMode::silica:
				return 3;
			case PluginMode::scale:
				return 4;
		}
		return 1;
	}

	constexpr PluginMode fromInt( const int &modeInt ) {
		switch ( modeInt ) {
			case 0:
				return PluginMode::pro;
			case 1:
				return PluginMode::melody;
			case 2:
				return PluginMode::drums;
			case 3:
				return PluginMode::silica;
			case 4:
				return PluginMode::scale;
			default:
				return PluginMode::melody;
		}
	}

	::juce::XmlElement PluginState::toXml() const {
		::juce::XmlElement root( "PeriodicLoopState" );

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
		root.setAttribute( "silicaMode", silicaMode );
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
		state.silicaMode   = xml.getBoolAttribute( "silicaMode", false );
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
				} );
			}
		}

		return state;
	}

} // namespace plop
