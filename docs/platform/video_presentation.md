# Video and presentation

[Platform index](../platform.md) · [INI persistence](settings.md) · [Renderer](../renderer_port.md)

## Settings

Defaults below are desktop defaults; Android/iOS use fullscreen and 4:3 fitting.
Environment overrides take precedence over the INI file.

- `video.fullscreen` / `P4PC_FULLSCREEN`: default 0 (mobile 1), borderless fullscreen.
  F11 or Alt+Enter toggles it during ordinary player input.
- `video.width`, `video.height` / `P4PC_WINDOW_W`, `P4PC_WINDOW_H`: default 640, 448;
  the resizable window's dimensions when not fullscreen.
- `video.resolution` / `P4PC_RESOLUTION`: default `scale:1`. Accepts `native`, `WxH`
  (each dimension 1..16384, also subject to GPU limits), or `scale:N` (N from 1 to 8).
- `video.aspect` / `P4PC_ASPECT`: default `4:3`; `16:9` gives a horizontal-plus camera
  view with an unstretched interface, and `auto` follows drawable aspect, including ultrawide.
  Aspect and physical resolution are independent policies.
- `video.movie_fit` / `P4PC_MOVIE_FIT`: default `bars`, retaining the original movie
  picture and bars. `zoom` fills the selected aspect uniformly with centred cropping
  (top/bottom for a wider target); `stretch` fills non-uniformly. The native Config
  **Movie fit** row applies immediately, independently of the camera/aspect setting.
- `video.fit` / `P4PC_PRESENT_FIT`: desktop default `stretch`, mobile `4:3`.
  `4:3` adds bars; `integer` fits the largest whole multiple of 640x480 with bars.
- `video.present` / `P4PC_PRESENT`: default `auto`. `blit` renders offscreen and fits to
  the drawable; `direct` draws into the window; `auto` selects the required path.
- `video.vsync` / `P4PC_VSYNC`: `on`, `off`, or default `auto` (on unless the display is
  known to run below 59 Hz). See pacing below.

No file/environment override leaves the 640x448 window and target drawn directly at
`scale:1`, the baseline the gates measure. Native/exact resolution, scale above one,
nondefault aspect, a larger window or fullscreen selects offscreen/blit presentation.
An incompatible `present=direct` is overridden with a diagnostic. The game retains
640x448 main virtual coordinates. Widescreen presentation preserves camera aspect,
rather than stretching its interface.

## Rendering resolution

`native` follows actual fitted drawable pixels across resizing, fullscreen and DPI changes.
Explicit `WxH` allocates exactly those physical pixels; `scale:N` scales the original or
aspect-adjusted storage. There is one resolution policy, not a separate scale setting.

`p4gl_readback_configure` consumes physical dimensions. Im2D coordinates, projection and
the GS half-pixel convention remain virtual. Frame feedback/DATE copies use the full
target and normalized UVs. Camera-texture targets, including shadows, scale on both axes
at the main picture's virtual-pixel density without changing guest dimensions.

The game's NEAREST/LINEAR sampler is retained on fractional grids. Smoothing nearest
atlas texels can erase glyph stems or alter alpha/depth/DATE results; resolution must not
silently change sampler rules. Axis-aligned 2D coverage follows GS pixel cells: a covered
1x sample fills its physical cell, including frame edges and letterboxes. The active
native sample grid is normally 640x448, or 853x448 at 16:9, while guest coordinates remain
640x448. Retail sprite sampling stays attached to the 1x sample through the UI canvas;
enhanced camera/frame feedback keeps continuous detail and virtual-center interpolation.
3D coverage stays continuous. Do not replace this with a global physical half-pixel offset.
Im3D line widths are virtual; non-native densities expand clipped segments to quads,
not optional GLES wide lines.

Captures retain physical dimensions. Canonical 640x448 references compare by exact area
reduction, not skipped checks; physical-size references compare directly.
`tools/scale_diff.py base.png scaled.png --json` reports per-axis ratios, reduced MAD
and physical-shift search without wrapping edges. Only its full-frame accelerator needs
NumPy; native gates and the fog metric do not.

## Present and window

`p4gl_context_present` fits the target into the output rectangle, clearing bars black.
Blits use nearest at 1:1 and linear otherwise. Native resolution reallocates storage at
frame boundaries after drawable-size changes; resizing alone never rebuilds the context.
Explicit/scale policies retain requested storage while fitting changes. Minimization keeps
the last valid target and skips presentation. Context recovery recreates physical storage.

`P4PC_TEST_RESIZE=frame:action,...` accepts `WxH`, `minimize`, `restore`, `fullscreen`
and `windowed`. `P4PC_CAPTURE_WINDOW=1` retains the presented drawable alongside each
capture as `<capture path>.win.png`. Offscreen resize fixtures do not qualify real-display
minimize/fullscreen/refresh-rate behavior.

The default GL API is the OpenGL ES 3.0 subset. `P4PC_GL_API=gl` selects desktop
OpenGL 3.3 core; API selection does not change the presentation contract.
For headless launch policy see [testing](testing.md#headless-execution).

## Pacing

The scheduler tick is 60 Hz, not the display refresh. Retail game updates/presents
normally run at 30 Hz on that tick, as on PS2; a ~33.3 ms flip interval is expected.
`p4pc_vsync_wait` waits for the next tick; deadlines advance by whole periods from the
first tick to avoid accumulated drift. A tick missed by a whole period resynchronizes
to now instead of replaying lost frames. When idle, the scheduler sleeps until 0.3 ms
before the tick and spins the remainder.

VSync only holds a finished frame for the display's next blank. `auto` disables that
wait below 59 Hz because a display period would otherwise slow the game. Tearing is
then possible. A 60 Hz display and the scheduler are not phase locked; an occasional
repeat/skip is possible. `P4PC_PACE_LOG=1` reports mean/longest period and the legacy
count over 20 ms every 600 flips; that count is not a 30 Hz hitch metric.
`P4PC_PACE_TRACE` provides individual intervals and host-thread CPU accounting;
classify hitches above approximately 40 ms. Virtual-clock/turbo runs disable swap
waiting; their deterministic scheduler and audio policy is described in
[performance](performance.md).
