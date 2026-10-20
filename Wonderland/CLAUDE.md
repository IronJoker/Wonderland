# CLAUDE.md

Wonderland: iPlug2 effect plugin (granulizer [free or tempo-synced stabs] → space/plate → width → mix → out).

## Structure

- `WonderlandDSP.h` - header-only DSP in `namespace ambi`. Buffers are allocated in `Prepare()` (called from `OnReset`), never in `ProcessBlock`.
- `WonderlandControls.h` - custom IGraphics vector controls in `namespace wui` (palette, `WKnob`, `WMiniKnob`, `WLedToggle`, `WSegment`, `WFader`, `WPlasmaT`, `WBackground`, `WPresetPill`).
- `Wonderland.cpp` - params, factory presets (`MakePresetFromNamedParams`; bools passed as int, others as double), layout, signal chain.

## UI

- Fixed 1040x640 design, zoomed as a whole via `AttachCornerResizer(EUIResizerMode::Scale)`. `mLayoutFunc` only attaches once.
- Host resizes come back through `OnParentWindowResize`/`ConstrainEditorResize`, which are overridden to keep the aspect ratio and convert size into zoom. Without that, VST3 hosts reset the zoom and the corner resizer appears broken.
- Flat 2D design: solid fills and thin lines only. No drop shadows, no gradients on controls.
- Fonts: Poppins (ExtraLight/Light/Regular/Medium) in `resources/fonts`, registered in `config.h` and `resources/main.rc`.
- The visualizer reads `GrainViz` (atomics written at the end of `ProcessBlock`).
- Controls animate hover/press by overriding `IsDirty()`. Keep all drawing inside each control's `mRECT` (glows/shadows included) or stale pixels are left behind on partial redraws.
- Controls derive from `wui::Activity`: they dim when their module is off or when a mode condition (`SetActiveWhen`) is not met.
- All user-facing text is lowercase (labels, param names, units, presets). `wui::Lower()` is applied to displayed values.
- Use `iplug::IPluginBase` explicitly in UI code; VST3 builds also have `Steinberg::IPluginBase`.

## Adding a parameter

1. Add to `EParams` (append before `kNumParams` to keep saved sessions compatible).
2. Init it in the constructor, set it in `UpdateDSPParams()`, and attach a control in `mLayoutFunc`.
