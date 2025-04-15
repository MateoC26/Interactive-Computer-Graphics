/* 
File Name: "vshader53.glsl":
Vertex shader:
  - Per vertex shading for a single point light source;
    distance attenuation is Yet To Be Completed.
  - Entire shading computation is done in the Eye Frame.
*/

#version 150  // YJC: Comment/un-comment this line to resolve compilation errors
              //      due to different settings of the default GLSL version

in  vec4 vPosition;
in  vec3 vNormal;
in  vec2 vTexCoord;

uniform mat4 model_view;
uniform mat4 projection;
uniform mat3 Normal_Matrix;

uniform vec4 GlobalAmbProd;

uniform vec4 DistDirection;
uniform vec4 DistAmbProd, DistDiffProd, DistSpecProd;

uniform vec4 PosPosition, PosDirection;
uniform vec4 PosAmbProd, PosDiffProd, PosSpecProd;
uniform float ConstAtt;    // Constant Attenuation
uniform float LinearAtt;   // Linear Attenuation
uniform float QuadAtt;     // Quadratic Attenuation
uniform float SLExponent;  // Spot Light Exponent
uniform float CutoffAngle; // Spot Light Cutoff Angle (Radians)

uniform float Shininess;

uniform vec4 ConstColor; // Uniform Color
uniform vec4 Viewer;     // Viewer Position

uniform int ObjFlag;               // 0:Floor, 1: Sphere, 2: Shadow, 3: Axis Lines
uniform int LightingFlag;          // 0:On, 1:Off
uniform int LightSourceFlag;       // 0:Point Source, 1:Spot Light
uniform int ConstColorFlag;        // 0:Non-const Color, 1:Const Color
uniform int SphereTexFlag;         // 0:Texture Mapping Disabled, 1: Contour Lines, 2: Checkerboard
uniform int SphereTexSlantFlag;    // 0:Vertical Texture, 1:Slanted Texture
uniform int SphereTexSpaceFlag;    // 0:Tex Coords in Object Space, 1:Tex Coords in Eye Space
uniform int SphereLatticeCompFlag; // 0:Upright Lattice, 1:Tilted Lattice

out vec4 color;
out float fogZ;
out vec2 texCoord;
out vec2 latticeTexCoord;

void main()
{
    gl_Position = projection * model_view * vPosition;

    fogZ = length((model_view * vPosition).xyz - (model_view * Viewer).xyz);

    // Texture Mapping
    if( ObjFlag == 1)
    {
        vec4 posit;
        float s, t;
        float ls, lt;

        // Texture Mapping Space (Object or Eye)
        if(SphereTexSpaceFlag == 0)
            posit = vPosition;
        else
            posit = (model_view * vPosition);

        // Sphere Texture Mapping Computations
        if(SphereTexFlag == 1)
        {
            if(SphereTexSlantFlag == 0)
            {
                s = 2.5 * posit.x;
                t = 0.0;
            }
            else
            {
                s = 1.5 * (posit.x + posit.y + posit.z);
                t = 0.0;
            }
        }
        else
        {
            if(SphereTexSlantFlag == 0)
            {
                s = 0.5 * (posit.x + 1);
                t = 0.5 * (posit.y + 1);
            }
            else
            {
                s = 0.3 * (posit.x + posit.y + posit.z);
                t = 0.3 * (posit.x - posit.y + posit.z);
            }
        }

        // Lattice Computations
        if(SphereLatticeCompFlag == 0)
        {
            ls = 0.5 * (vPosition.x + 1);
            lt = 0.5 * (vPosition.y + 1);
        }
        else
        {
            ls = 0.3 * (vPosition.x + vPosition.y + vPosition.z);
            lt = 0.3 * (vPosition.x - vPosition.y + vPosition.z);
        }

        texCoord = vec2(s, t);
        latticeTexCoord = vec2(ls, lt);
    }
    else if(ObjFlag == 2)
    {   
        float ls, lt;

        // Lattice Computations
        if(SphereLatticeCompFlag == 0)
        {
            ls = 0.5 * (vPosition.x + 1);
            lt = 0.5 * (vPosition.y + 1);
        }
        else
        {
            ls = 0.3 * (vPosition.x + vPosition.y + vPosition.z);
            lt = 0.3 * (vPosition.x - vPosition.y + vPosition.z);
        }

        latticeTexCoord = vec2(ls, lt);
        texCoord = vTexCoord;
    }
    else
        texCoord = vTexCoord;
    
    // Shading
    if(LightingFlag == 0 || ConstColorFlag == 1)
    {
        color = ConstColor;
    }
    else
    {
        // Transform vertex  position into eye coordinates
        vec3 pos = (model_view * vPosition).xyz;
	
        vec3 dist_L = normalize( -DistDirection.xyz );
        vec3 posl_L = normalize( PosPosition.xyz - pos );
        vec3 E = normalize( -pos );
        vec3 dist_H = normalize( dist_L + E );
        vec3 posl_H = normalize( posl_L + E);

        vec3 N = normalize(Normal_Matrix * vNormal);

        // YJC Note: N must use the one pointing *toward* the viewer
        //     ==> If (N dot E) < 0 then N must be changed to -N
        //
        if ( dot(N, E) < 0 ) N = -N;

        /* ------Global Ambient------ */
        vec4 global_ambient = GlobalAmbProd;

        /* ------Distant Directional------ */
        float attenuation = 1.0; 

        vec4 ambient = DistAmbProd;

        float d = max( dot(dist_L, N), 0.0 );
        vec4  diffuse = d * DistDiffProd;

        float s = pow( max(dot(N, dist_H), 0.0), Shininess );
        vec4  specular = s * DistSpecProd;
    
        if( dot(N, dist_L) < 0.0 ) {
	    specular = vec4(0.0, 0.0, 0.0, 1.0);
        } 

        vec4 distant_directional = attenuation * (ambient + diffuse + specular);

        /* ------Positional------ */
        float distance = 0.0f;

        distance = length( pos - PosPosition.xyz );

        attenuation = 1 / (ConstAtt + LinearAtt * distance + QuadAtt * pow(distance, 2));
        
        vec3 posDir = normalize(PosDirection.xyz);

        // Spot Light
        if(LightSourceFlag == 1)
        {
            if(dot(posDir, -posl_L) >= cos(CutoffAngle))
            {
                attenuation *= pow(dot(posDir, -posl_L), SLExponent);
            }
            else
            {
                attenuation *= 0;
            }
        }

        ambient = PosAmbProd;

        d = max( dot(posl_L, N), 0.0 );
        diffuse = d * PosDiffProd;

        s = pow( max(dot(N, posl_H), 0.0), Shininess );
        specular = s * PosSpecProd;
    
        if( dot(posl_L, N) < 0.0 ) {
	    specular = vec4(0.0, 0.0, 0.0, 1.0);
        } 

        vec4 positional = attenuation * (ambient + diffuse + specular);

        /*------Final------ */

        gl_Position = projection * model_view * vPosition;

        color = global_ambient + distant_directional + positional;
    }
}
