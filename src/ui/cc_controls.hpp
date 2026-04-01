#ifndef PLOP_SRC_UI_CC_CONTROLS_HPP
#define PLOP_SRC_UI_CC_CONTROLS_HPP

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ui/colours.hpp"
#include "ui/knob.hpp"
#include "ui/ui_constants.hpp"

namespace plop::ui {

   /// A row of 3 rotary knobs for CC-related controls.
   /// Call configureKnob() and setKnobValue() to wire up.
   class CcControls : public ::juce::Component {
    public:
      static constexpr int NUM_KNOBS = 3;

      CcControls() {
         static constexpr const char *DEFAULT_LABELS[ NUM_KNOBS ] = { "Period", "Depth", "Offset" };
         for ( int i = 0; i < NUM_KNOBS; ++i ) {
            mKnobs[ static_cast<size_t>( i ) ] = std::make_unique<Knob>();
            mKnobs[ static_cast<size_t>( i ) ]->setLabel( DEFAULT_LABELS[ i ] );
            addAndMakeVisible( *mKnobs[ static_cast<size_t>( i ) ] );
         }
      }

      /// Set label, value range, and onChange callback for one knob (index 0–2).
      void configureKnob( int index, const ::juce::String &label, float min, float max, std::function<void( float )> onChange ) {
         if ( index < 0 || index >= NUM_KNOBS )
            return;
         mKnobs[ static_cast<size_t>( index ) ]->setLabel( label );
         mKnobs[ static_cast<size_t>( index ) ]->setRange( min, max );
         mKnobs[ static_cast<size_t>( index ) ]->setOnChange( std::move( onChange ) );
      }

      void setKnobValue( int index, float value ) {
         if ( index >= 0 && index < NUM_KNOBS )
            mKnobs[ static_cast<size_t>( index ) ]->setValue( value );
      }

      float getKnobValue( int index ) const {
         if ( index >= 0 && index < NUM_KNOBS )
            return mKnobs[ static_cast<size_t>( index ) ]->getValue();
         return 0.0f;
      }

      void paint( ::juce::Graphics &g ) override {
         g.fillAll( colours::panelBg );
         g.setColour( colours::borderLine );
         g.drawHorizontalLine( 0, 0.0f, static_cast<float>( getWidth() ) );
      }

      void resized() override {
         const int halfW = getWidth() / 2;
         const int halfH = getHeight() / 2;
         mKnobs[ 0 ]->setBounds( 0,     0,     halfW,            getHeight() );
         mKnobs[ 1 ]->setBounds( halfW, 0,     getWidth() - halfW, halfH );
         mKnobs[ 2 ]->setBounds( halfW, halfH, getWidth() - halfW, getHeight() - halfH );
      }

    private:
      std::array<std::unique_ptr<Knob>, NUM_KNOBS> mKnobs;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( CcControls )
   };

} // namespace plop::ui

#endif // PLOP_SRC_UI_CC_CONTROLS_HPP
