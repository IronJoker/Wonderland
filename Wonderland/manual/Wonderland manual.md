# wonderland manual

wonderland by simi soprano is a free, dreamy ambience plugin: a granulizer that floats freely or chops in time with your daw's bpm, plus space (plate reverb) and width.

## signal flow

input → granulizer → space → width → mix → out

## the controls

- **grains** (big knob): how much granulizer you hear. 100 % = only grains.
- **free / sync**
  - free: grains float on their own. **length** sets how long each grain is.
  - sync: grains become stabs locked to your daw's tempo. pick a **rate** (1/2 to 1/32) and a **feel** (straight, triplet, dotted). **gate** sets how long each stab rings before the gap (low = short and choppy, high = long).
- **chaos**: randomness. adds stereo movement, little pitch drifts and, in sync mode, stutter repeats and octave jumps.
- **tune**: pitch of the grains, in semitones (0.5 cent steps).
- **space**: plate reverb. turning it up makes the room bigger and wetter.
- **width**: 0 % = mono, 100 % = unchanged, 200 % = extra wide (bass stays mono).
- **mix**: blends the whole effect with the dry signal.
- **out**: output volume.

grains follow the live input with only a few milliseconds of delay. in sync mode stabs land exactly on the beat grid while your daw is playing, and keep running at the session tempo when it's stopped.

## the visualizer

the waves swell with the granulizer. each dot is a live grain: left/right shows its stereo position, height and colour show its pitch. in sync mode the line along the bottom shows progress through the current step.

## tips

- drag or scroll any knob, hold shift for fine control.
- right-click (or double-click) any knob or button to reset it. knobs go back to zero, except mix (100 %), width (100 % = unchanged stereo), length (160 ms) and gate (50 %). the buttons go back to free, 1/8 and straight.
- to type an exact value, double-click or right-click the number under a knob. tune accepts up to two decimals (for example 1.33 or 1,33) and moves in 0.5 cent steps.
- presets live top right: the arrows step through them, clicking the name opens the list, right-click goes back to "default".
- drag the bottom-right corner to zoom the window (double-click it to reset).
