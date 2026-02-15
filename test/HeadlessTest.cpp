#include <cassert>
#include <iostream>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>


/// Headless test runner for Algo Nebula.
/// Phase 1: basic smoke test — just verify the build compiles and links.
int main() {
  std::cout << "=== Algo Nebula Headless Tests ===" << std::endl;
  std::cout << "Phase 1: Build verification" << std::endl;

  // Verify JUCE core is functional
  juce::String testStr("AlgoNebula");
  assert(testStr.isNotEmpty());

  // Verify audio buffer allocation
  juce::AudioBuffer<float> buffer(2, 512);
  assert(buffer.getNumChannels() == 2);
  assert(buffer.getNumSamples() == 512);

  buffer.clear();
  for (int ch = 0; ch < 2; ++ch)
    for (int s = 0; s < 512; ++s)
      assert(buffer.getSample(ch, s) == 0.0f);

  std::cout << "All Phase 1 tests passed." << std::endl;
  return 0;
}
