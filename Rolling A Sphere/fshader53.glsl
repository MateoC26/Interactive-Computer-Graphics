/* 
File Name: "fshader53.glsl":
           Fragment Shader
*/

#version 150  // YJC: Comment/un-comment this line to resolve compilation errors
              //      due to different settings of the default GLSL version

in  vec4 color;
in  float fogZ;
in  vec2 texCoord;
in  vec2 latticeTexCoord;

uniform vec4 FogColor;     // Color of Fog
uniform float FogStart;    // Linear Fog Start
uniform float FogEnd;      // Linear Fog End
uniform float FogDensity;  // Exponential (Normal and Squared) Fog

uniform int ObjFlag;            // 0:Floor, 1: Sphere, 2: Shadow, 3: Axis Lines
uniform int FogFlag;            // 0: Off, 1: Linear, 2: Exponential, 3: Exponential Squared
uniform int FloorTexFlag;       // 0: Texture Mapping Disabled, 1: Texture Mapping Enabled
uniform int SphereTexFlag;      // 0: Texture Mapping Disabled, 1: Contour Lines, 2: Checkerboard
uniform int SphereFillFlag;     // 0: Wireframe Sphere (No Fill), 1: Filled Sphere
uniform int SphereLatticeFlag;  // 0:Lattice Effect Off, 1:Lattice Effect On

uniform sampler2D texture_2D; 
uniform sampler1D texture_1D;

out vec4 fColor;

void main() 
{ 
    float f;
    vec4 col;

    if(FogFlag == 0)
    {
        f = 1;
    }
    else if(FogFlag == 1)
    {
        f = (FogEnd - fogZ) / (FogEnd - FogStart);
    }
    else if(FogFlag == 2)
    {
        f = exp(-(FogDensity * fogZ));
    }
    else if(FogFlag == 3)
    {
        f = exp(-pow((FogDensity * fogZ), 2));
    }
    
    f = clamp(f, 0, 1);

    if(ObjFlag == 0)
    {
        if (FloorTexFlag == 0)
        {
            col = color;
        }
        else if(FloorTexFlag == 1)
        {
            col = color * texture( texture_2D, texCoord );
        }
    }
    else if(ObjFlag == 1)
    {
        if(SphereFillFlag == 0 || SphereTexFlag == 0)
        {
            col = color;
        }
        else if(SphereTexFlag == 1)
        {
            col = color * texture( texture_1D, texCoord.x );
        }
        else
        {
            vec4 texColor = texture( texture_2D, texCoord );

            if(texColor.x == 0.0)
                texColor = vec4(0.9, 0.1, 0.1, 1.0);

            col = color * texColor;
        }

        if(SphereFillFlag != 0 && SphereLatticeFlag == 1)
        {
            float fracS = fract(4 * latticeTexCoord.x);
            float fracT = fract(4 * latticeTexCoord.y);

            if(fracS < 0.35 && fracT < 0.35)
                discard;
        }
    }
    else if(ObjFlag == 2)
    {
        if(SphereFillFlag != 0 && SphereLatticeFlag == 1)
        {
            float fracS = fract(4 * latticeTexCoord.x);
            float fracT = fract(4 * latticeTexCoord.y);

            if(fracS < 0.35 && fracT < 0.35)
                discard;
        }

        col = color;
    }
    else
    {
        col = color;
    }

    fColor = (f * col) + ((1 - f) * FogColor);
} 

