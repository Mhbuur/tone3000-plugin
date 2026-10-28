// EQ editor math (ports of eqMath.ts and the numeric half of eqShared.ts):
// the RBJ biquad response the DSP runs (plugin/src/BlockEq.cpp, A =
// 10^(dB/40)) so the drawn curve is the audio truth, the log frequency axis,
// the gain axis of the shared graph space, and the readout formats.
#pragma once

#include <juce_core/juce_core.h>

#include <vector>

#include "core/Wheel.h"
#include "model/ChainState.h"

namespace t3k::ui::eq {

// Combined magnitude response (dB) of the active bands at each frequency.
// Inert bands are skipped, matching the audio thread.
std::vector<double> responseDb(const std::vector<EqBand>& bands, double sampleRate,
                               const std::vector<double>& freqsHz);

// Frequency ↔ 0..1 position on the log-scaled x axis (20 Hz .. 20 kHz).
double freqToNorm(double freqHz);
double normToFreq(double norm);

// "251 Hz" / "1.60k" / "12.5k".
juce::String formatFreq(double freqHz);
// Typed frequency: plain Hz ("800") or k-notation ("1.2k"); nullopt when
// unparsable.
std::optional<double> parseFreq(const juce::String& raw);
// Typed decimal ("," accepted as the separator).
std::optional<double> parseDecimal(const juce::String& raw);

bool hasGain(EqBandType type);
juce::String typeLabel(EqBandType type);
// 16x14 curve glyph for the type selector / band labels: the bare path,
// and a white-stroked SVG (stroke 1.6, round caps) for Icons::draw. Both
// point at static storage so the icon cache can key on the pointer.
const char* typeGlyphPath(EqBandType type);
const char* typeGlyphSvg(EqBandType type);
// Q applied when switching to `type` (cuts and shelves reset to 0.71; bells
// keep theirs).
std::optional<double> typeDefaultQ(EqBandType type);

// Shared graph space (eqShared.ts)
// Full card body: outer card minus 1px border each side and the chrome
// header. Both EQ views draw into this space so the spectrum lines up.
inline constexpr int kGraphW = 800 - 2;
// The body is laid out 275 tall (the card's border hides its last 2px).
// Graph space is 273 tall: the web draws grid, curve, dots and spectrum in
// an SVG with that viewBox and stretches it over the whole body
// (preserveAspectRatio none), so graph y times kSvgStretch is body y.
inline constexpr int kBodyH = 275;
// eqShared.ts GRAPH_H: the card minus its 2px border and the 45px header,
// which is the body less the 2px hidden under the bottom border.
inline constexpr int kGraphH = kBodyH - 2;
inline constexpr float kSvgStretch = static_cast<float>(kBodyH) / kGraphH;
inline constexpr int kGraphPadY = 12;  // keeps dots inside the frame at ±15 dB

inline double gainToY(double gainDb) {
  return kGraphH / 2.0 - (gainDb / kEqMaxAbsGainDb) * (kGraphH / 2.0 - kGraphPadY);
}
inline double yToGain(double y) {
  return ((kGraphH / 2.0 - y) / (kGraphH / 2.0 - kGraphPadY)) * kEqMaxAbsGainDb;
}

// Fader view (EqSliders.tsx), in the same body space as the curve. The
// pad is shared with the Graph's chrome row and band readout; the rest is
// the fader's own layout, kept here so its hit test is testable without a
// Component.
inline constexpr int kBodyPad = 16;
inline constexpr int kFaderThumbPad = 7;  // ceil(capH / 2): keeps the cap inside the travel
inline constexpr int kFaderFreqRowH = 16;
inline constexpr int kFaderAxisInset = 4;

// The region a fader can be grabbed in: the body less the top pad and the
// frequency-label row. Wider than travelBounds() by the thumb pad each end,
// so a grab just outside the cap still lands.
inline juce::Rectangle<float> faderRegion() {
  return {static_cast<float>(kBodyPad), static_cast<float>(kBodyPad),
          static_cast<float>(kGraphW - 2 * kBodyPad),
          static_cast<float>(kBodyH - kBodyPad - kFaderAxisInset - kFaderFreqRowH)};
}

// Fader column under a point, or -1 outside the fader region. The column
// width is the design-time kEqNumBands, exactly as the paint lays the faders
// out, so a hit test and its pixels cannot disagree; `bandCount` only clamps
// the index to the bands actually present.
inline int faderColumnAt(juce::Point<float> pos, int bandCount) {
  if (bandCount <= 0 || !faderRegion().contains(pos)) return -1;
  const float width = static_cast<float>(kGraphW - 2 * kBodyPad) / static_cast<float>(kEqNumBands);
  const int col = static_cast<int>((pos.x - kBodyPad) / width);
  return juce::jlimit(0, bandCount - 1, col);
}

// One click's gain change, in dB. Settled on hardware over three passes: 1 dB
// overshot a normal ear move, 0.5 dB lands on the readout's own smallest step.
inline constexpr double kFaderDbPerNotch = 0.5;

// One wheel event's gain change in dB. The sign matches the curve's, which
// multiplies Q by exp(-pixelsY * 0.003): pixelsY is the web deltaY, so
// positive is scroll *down*, and the negated sign means scrolling up boosts.
// Both surfaces then move the same way for the same gesture, on one screen.
// Shift divides by wheel::kFineDivisor, the UI's shared fine factor.
inline double faderGainDb(float pixels, bool fine) {
  const double notches = static_cast<double>(pixels) / static_cast<double>(wheel::kNotchPixels);
  return -notches * kFaderDbPerNotch / (fine ? wheel::kFineDivisor : 1.0f);
}

}  // namespace t3k::ui::eq
