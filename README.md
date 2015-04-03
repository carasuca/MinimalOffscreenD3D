# MinimalOffscreenD3D
Minimal C++ project for offscreen image rendering using D3D11 and WIC.

As simple as it gets, this native C++ console app generates a PNG
using API from Windows 8 SDK plus some cool Visual Studio 2013 features.
There are no windows created, no WinAPI calls, only:
- Direct3D 11,
- Windows Imaging Components (WIC),
- integrated HLSL compiler,
- stl.

### Steps

1. Create D3D11 device and context.
2. Create offscreen buffer.
3. Load and set shaders.
4. Draw a simple quad.
5. Copy texture from render target.
5. Create png encoder and stream.
6. Save image buffer.
