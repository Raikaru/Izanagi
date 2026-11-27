# Renderer

Izanagi replaces the PS2 sky2 device and pipeline seams with a native renderer.
The game and RenderWare still own scene traversal, materials and draw ordering;
port code translates their requests into graphics operations. The implementation
is in `src/render/`, with RenderWare-facing overrides in `src/platform/` and
`overrides/`.

## RenderWare and GS behavior

The device supplies RenderWare's system requests, render-state operations and
standard functions. Pipeline nodes handle the platform-specific work that would
otherwise produce PS2 DMA, GIF and VIF packets. Port changes should preserve the
game's guest data layouts and the ordering of its draws, not translate hardware
packets instruction by instruction.

Materials animate through RpMatFX: the game's UV animation interpolators rewrite a
material's UV transform matrix every frame (eye and mouth sheets, scrolling TV and
portal surfaces, battle arena backdrops). The atomic and world-sector draws read
that matrix from the material's matfx record and apply it to the vertex UVs before
sampling, as the GS pipeline did. `render_smoke` checks the frame sequences of a
stepped (blink) and a scrolling (portal, arena) transform on both draw paths.

GS behavior that matters to output includes alpha and destination-alpha tests,
colour and depth write masks, blending, indexed texture and palette handling,
and the pixel-centre convention. Texture sampling and raster coverage must remain
consistent when rendering at enhanced resolution. See
[video and presentation](platform/video_presentation.md) for the player-facing
resolution, aspect and presentation settings.

Cameras have a projection type. Most are perspective; the field's shadow camera is parallel (`RwCameraSetProjectionType(camera, 2)`):
its x and y are camera-space coordinates over the view window with no divide by depth, and depth is linear between its near and far planes.
`p4gl_camera_object_matrix` builds the matrix for both, so a draw into the 128x128 shadow texture stays the size the game placed it.
A perspective matrix there shrinks the player to one texel and the ground shadow disappears.

## 10. OpenGL ES 3.0 path (one renderer source for Windows, Linux, macOS, Android and iOS)

The default renderer uses the OpenGL ES 3.0 subset. Set `P4PC_GL_API=gl` to
request desktop OpenGL 3.3 core where available; `P4PC_GL_API=gles` selects the
ES path. macOS uses ANGLE for its ES path, with desktop OpenGL as a fallback
when an ES display is unavailable. See [macOS](platform/macos.md) for package
and platform limitations.

## Resolution and widescreen

The game's main virtual coordinates remain 640×448. Resolution controls change
physical render-target size, not guest coordinates. Widescreen changes the
camera's horizontal view while retaining the interface layout; presentation
then fits the render target to the drawable. Native-size captures compare
directly, and scaled captures require an appropriate reduction before comparing
against a native-size reference. See [video and presentation](platform/video_presentation.md).

## Testing and diagnostics

The renderer smoke tests exercise real renderer paths and require a graphics
context. Tests that load game files require a disc image and executable supplied
by the user; image-comparison tests also need their reference captures. Those
files and resulting captures do not belong in the repository. See
[testing](platform/testing.md) and [BUILDING.md](../BUILDING.md) for the public
build and test workflow. A successful build or synthetic fixture alone does not
establish visual or gameplay equivalence on every GPU.
