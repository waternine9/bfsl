# Welcome to BFSL
> The public domain shading language for the CPU   

## Why use BFSL over other alternatives?
- BFSL is public domain
- BFSL is very easy to set up
- BFSL is beginner friendly
- BFSL is multithreaded, can run in background

## Alright, but how do I use it?

It's as simple as 123!
1. Link the static library found in the releases with your build system
2. Include the header file, `shading.hpp'
3. Start using it!

## Tutorial (for shader code)

The syntax is *mostly* like GLSL, but with a few twists.

- Floating-point constants must be prefixed with `0f`
- No implicit casting
- Integral constants are deprecated
- There is no function calling **yet** (which means there is also no return statements **yet**)
- There is no comments
- There are no `for` loops
- Shader code must terminated by `$` (anything after the `$` is ignored)
- Top-level statements are not allowed, only function declarations are allowed

Here's the built-in types:

- `float`
- `vec2`
- `vec3`
- `vec4`
- `tex2d`
- `int` (deprecated)

To use these types, you must do `{type}:{variable name}`, same goes for functions.  
Functions are declared like in GLSL, here's an example:
```
int:main(vec3:FragCoord)
{
  ...
}
```

### Quick note
If you are making a fragment shader, your main function will have to accept a `vec2:FragCoord` (current pixel position in the range of `[0, resolution]` and `vec3:OutColor` (normalized output color).  
If you are making any other kind of shader, there are no requirements
### End Quick Note

The operators available are:
- `+`
- `-`
- `*`
- `/`
- `=`
There is also conditional operators, which unlike other operators that can work with any type, conditionals only work for floats, and can only be used in `while` loops or `if` statements
- `==`
- `<`
- `>`

Also, you can do swizzling just like in GLSL, but **you can also do swizzling with floats**, like so:
```
int:main()
{
  float:Var = 0f10;
  vec3:Var3 = Var.xxx;
}
$
```

There is also loops and conditionals, and only `while` loops and single-condition `if` statements are in the language so far.  
To use them, its the same as GLSL, except:
- Only 1 condition allowed for `while` and `if`
- Closing brace must be terminated with `;`

The built-in functions are:
- `dot`: Computes the dot product of any 2 vectors. Returns a `float`
- `cross`: Computes the cross product of 2 `vec3`'s. Returns a `vec3`
- `length`: Computes the magnitude of any vector. Returns a `float`
- `abs`: Returns the absolute of any input component-wise
- `sin`: Computes the sine of a `float` input. Returns a `float`
- `cos`: Computes the cosine of a `float` input. Returns a `float`
- `sample`: Samples a pixel from the first input (must be `tex2d`) with the second input (must be `vec2`). Returns a `vec3`
-- Also, the second input is expected to be in the range of 0 to 1 on the X and Y components, otherwise it will repeat the texture
- `clamp01` Takes in a `float` input and clamps it to 0 to 1. Returns a `float`

## Tutorial (for host code)

Here's example code that will explain everything, and don't be intimidated, it's 99% comments :-)

```c++
#include <iostream>
#include "shading.hpp"

int main(void)
{
  ShadingLang::Manager ShaderMan; // This class is for managing fragment shaders, and only fragment shaders
	ShaderMan.Use("frag.bf"); // This member function  takes in a path to a shader file and compiles it for later use
  // Let's say frag.bf has a variable called `SomeVar`, and we want to access it. We will have to call the `GetVar` member function
  ShadingLang::Variable* SomeVar = ShaderMan.GetVar("main", "SomeVar");
  SomeVar->X = 10.0f; // Now we can directly access the contents of the variable
  // As an example, let's also bind a texture to a `tex2d` variable in frag.bf
  ShadingLang::Variable* SomeTex = ShaderMan.GetVar("main", "SomeTex"); // `SomeTex` should be of type `tex2d`
  SomeVar->Tex.Data = NULL; // You can't assign NULL to the texture data in actual code. The format of the texture data is 3 `float`s per pixel
  SomeVar->Tex.ResX = 128; // Here should go the width of your texture data, 128 is just for example
  SomeVar->Tex.ResY = 128; // Here should go the height of your texture data, 128 is just for example
  // Now let's execute the shader. The `Execute` member function takes in 4 arguments:
  // - ResX: Width of the output framebuffer
  // - ResY: Height of the output framebuffer
  // - OutBuff: The output framebuffer, must have a size of ResX * ResY * 4 bytes
  // - DepthBuff: The input depth buffer, must have a size of ResX * ResY * sizeof(float) bytes. 
  // -- The reason this is needed, is to know where to shade pixels. 
  // -- The manager won't render pixels with a depth value of 0.0f
  ShaderMan.Execute(256, 256, NULL, NULL); // I put in NULL for the buffers for simplicity, you can't do this in actual code.
  ShaderMan.Wait(256, 256); // The `Wait` member function waits for all threads to finish rendering. The first 2 arguments to this function should be the same as the first 2 for the `Execute` member function
  // Now the passed OutBuff contains the colors outputted from the fragment shader. Now let's print the X component of `SomeVar`
  std::cout << SomeVar.X << "\n";
  
  // Now lets execute a vertex shader. For this shader, it's highly recommended to use the `SingleManager` class, since it's not a fragment shader. Note though that it is single-threaded
  ShadingLang::SingleManager VertShaderMan;
	ShaderMan.Use("vert.bf"); // Same as before
  // Let's pass in an example vertex
  ShadingLang::Variable* InCoord = VertShaderMan.GetVar("main", "InCoord");
  InCoord->X = 0.5f;
  InCoord->Y = -0.4f;
  InCoord->Z = 0.2f;
  // Now execute (it executes synchronously, unlike the fragment shader
  VertShaderMan.Execute();
  // Now let's print the output vertex
  std::cout << InCoord->X << " " << InCoord->Y << " " << InCoord->Z << "\n";
  
  return 0;
}
