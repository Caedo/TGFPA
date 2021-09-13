# TGFPA - Texture Generator For Programmer Art

TGFPA is simple texture generation tool, designed for fast prototyping.

How to Use
==========

Crate new shader file (File > New...), open in text editor of your choise and start editing. TGFPA support hot-reloading, so all your changes will be immediatly visible (keep in mind, that currently variable values aren't saved between compilation - that is very high on priority list). 

Shader file have to be valid [GLSL](https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)) program, with main function having signature: `void mainImage( out vec4 fragColor, in vec2 fragCoord )` (the same as [Shadertoy](https://www.shadertoy.com/)). 

TGFPA creates specific UI controls for all uniform variables (the ones with greyish tint are stripped by compiler). What controls are used can be altered with custom attributes: `DRAG()`, `Range(min, max)`, `Color()` (only for vec3 and vec4).

### Libs
TGFPA uses custom, #include-like system for adding libraries to shaders. Library file have to be inside `shader/lib` directory. To include library use `LIB("<library-name>")`, keep in mind that it literally pastes library file on top of shader file, so all name collisions can occur. 

Build
=====
Currently only building with Visual Studio is supported. To build run a x64 Developer Command Prompt (alternatively you can have [vcvarsall](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160#developer_command_file_locations) in your PATH) and then run `build.bat` script. To build release version run the script with `release` parameter.

Linux builds will be (hopefully) supported in the future.
