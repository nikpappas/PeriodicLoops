#ifndef PLOP_SRC_UI_KNOB_CC_VALUE_HPP
#define PLOP_SRC_UI_KNOB_CC_VALUE_HPP

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ui/colours.hpp"
#include "ui/knob.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

	/// A Knob (range 0–127) that displays the standard MIDI CC controller name below the dial.
	class KnobCcValue : public ::juce::Component {
	 public:
		KnobCcValue( const ::juce::String &label, std::function<void( float )> onChange ) :
				  mKnob( label, std::move( onChange ) ) {
			mKnob.setRange( 0.0f, 127.0f );
			addAndMakeVisible( mKnob );
		}

		void setLabel( const ::juce::String &label ) {
			mKnob.setLabel( label );
		}

		void setValue( float v ) {
			mKnob.setValue( v );
			repaint();
		}

		float getValue() const {
			return mKnob.getValue();
		}

		void setOnChange( std::function<void( float )> onChange ) {
			mKnob.setOnChange( std::move( onChange ) );
		}

		void setActive( bool active ) {
			mKnob.setActive( active );
			repaint();
		}

		bool isActive() const {
			return mKnob.isActive();
		}

		void resized() override {
			mKnob.setBounds( 0, 0, getWidth(), getHeight() - VALUE_H );
		}

		void paint( ::juce::Graphics &g ) override {
			if ( !mKnob.isActive() )
				return;

			g.setColour( ::juce::Colours::black );
			g.setFont( ::juce::Font( FONT_SM ) );
			g.drawText( ccName( static_cast<int>( mKnob.getValue() ) ),
			            0,
			            getHeight() - VALUE_H,
			            getWidth(),
			            VALUE_H,
			            ::juce::Justification::centred );
		}

	 private:
		static constexpr int VALUE_H = static_cast<int>( FONT_SM ) + 6;

		static ::juce::String ccName( int cc ) {
			// clang-format off
			static constexpr const char *NAMES[ 128 ] = {
			    /* 0  */ "Bank Sel",
			    /* 1  */ "Mod",
			    /* 2  */ "Breath",
			    /* 3  */ "CC 3",
			    /* 4  */ "Foot",
			    /* 5  */ "Portamento",
			    /* 6  */ "Data Entry",
			    /* 7  */ "Volume",
			    /* 8  */ "Balance",
			    /* 9  */ "CC 9",
			    /* 10 */ "Pan",
			    /* 11 */ "Expression",
			    /* 12 */ "FX Ctrl 1",
			    /* 13 */ "FX Ctrl 2",
			    /* 14 */ "CC 14",
			    /* 15 */ "CC 15",
			    /* 16 */ "General 1",
			    /* 17 */ "General 2",
			    /* 18 */ "General 3",
			    /* 19 */ "General 4",
			    /* 20 */ "CC 20",   /* 21 */ "CC 21",   /* 22 */ "CC 22",   /* 23 */ "CC 23",
			    /* 24 */ "CC 24",   /* 25 */ "CC 25",   /* 26 */ "CC 26",   /* 27 */ "CC 27",
			    /* 28 */ "CC 28",   /* 29 */ "CC 29",   /* 30 */ "CC 30",   /* 31 */ "CC 31",
			    /* 32 */ "Bank LSB",/* 33 */ "Mod LSB", /* 34 */ "Breath L", /* 35 */ "CC 35",
			    /* 36 */ "Foot LSB",/* 37 */ "Port LSB",/* 38 */ "Data LSB", /* 39 */ "Vol LSB",
			    /* 40 */ "Bal LSB", /* 41 */ "CC 41",   /* 42 */ "Pan LSB",  /* 43 */ "Expr LSB",
			    /* 44 */ "CC 44",   /* 45 */ "CC 45",   /* 46 */ "CC 46",    /* 47 */ "CC 47",
			    /* 48 */ "CC 48",   /* 49 */ "CC 49",   /* 50 */ "CC 50",    /* 51 */ "CC 51",
			    /* 52 */ "CC 52",   /* 53 */ "CC 53",   /* 54 */ "CC 54",    /* 55 */ "CC 55",
			    /* 56 */ "CC 56",   /* 57 */ "CC 57",   /* 58 */ "CC 58",    /* 59 */ "CC 59",
			    /* 60 */ "CC 60",   /* 61 */ "CC 61",   /* 62 */ "CC 62",    /* 63 */ "CC 63",
			    /* 64 */ "Sustain",
			    /* 65 */ "Portamento",
			    /* 66 */ "Sostenuto",
			    /* 67 */ "Soft Pedal",
			    /* 68 */ "Legato",
			    /* 69 */ "Hold 2",
			    /* 70 */ "Sound Var",
			    /* 71 */ "Resonance",
			    /* 72 */ "Release",
			    /* 73 */ "Attack",
			    /* 74 */ "Cutoff",
			    /* 75 */ "Decay",
			    /* 76 */ "Vib Rate",
			    /* 77 */ "Vib Depth",
			    /* 78 */ "Vib Delay",
			    /* 79 */ "CC 79",
			    /* 80 */ "General 5",
			    /* 81 */ "General 6",
			    /* 82 */ "General 7",
			    /* 83 */ "General 8",
			    /* 84 */ "Port Ctrl",
			    /* 85 */ "CC 85",   /* 86 */ "CC 86",   /* 87 */ "CC 87",   /* 88 */ "CC 88",
			    /* 89 */ "CC 89",   /* 90 */ "CC 90",
			    /* 91 */ "Reverb",
			    /* 92 */ "Tremolo",
			    /* 93 */ "Chorus",
			    /* 94 */ "Detune",
			    /* 95 */ "Phaser",
			    /* 96 */ "Data +1",
			    /* 97 */ "Data -1",
			    /* 98 */ "NRPN LSB",
			    /* 99 */ "NRPN MSB",
			    /* 100 */ "RPN LSB",
			    /* 101 */ "RPN MSB",
			    /* 102 */ "CC 102",  /* 103 */ "CC 103",  /* 104 */ "CC 104",  /* 105 */ "CC 105",
			    /* 106 */ "CC 106",  /* 107 */ "CC 107",  /* 108 */ "CC 108",  /* 109 */ "CC 109",
			    /* 110 */ "CC 110",  /* 111 */ "CC 111",  /* 112 */ "CC 112",  /* 113 */ "CC 113",
			    /* 114 */ "CC 114",  /* 115 */ "CC 115",  /* 116 */ "CC 116",  /* 117 */ "CC 117",
			    /* 118 */ "CC 118",  /* 119 */ "CC 119",
			    /* 120 */ "All Snd Off",
			    /* 121 */ "Reset All",
			    /* 122 */ "Local",
			    /* 123 */ "All Notes Off",
			    /* 124 */ "Omni Off",
			    /* 125 */ "Omni On",
			    /* 126 */ "Mono",
			    /* 127 */ "Poly",
			};
			// clang-format on
			if ( cc < 0 || cc > 127 )
				return "CC ?";
			return NAMES[ cc ];
		}

		Knob mKnob;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( KnobCcValue )
	};

} // namespace plop::ui

#endif // PLOP_SRC_UI_KNOB_CC_VALUE_HPP
