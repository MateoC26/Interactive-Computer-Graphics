#include "Angel-yjc.h"

typedef Angel::vec3  point3;
typedef Angel::vec4  point4;
typedef Angel::vec4  color4;

GLuint Angel::InitShader(const char* vShaderFile, const char* fShaderFile);

/* Shader Program Object ID*/
GLuint program;
GLuint programF;

/* Vertex Buff Object IDs */
GLuint sphere_buffer; 
GLuint sphere_shadow_buffer; 
GLuint floor_buffer;  
GLuint x_axis_buffer; 
GLuint y_axis_buffer; 
GLuint z_axis_buffer; 
GLuint firework_buffer;

/* Projection transformation parameters */
GLfloat  fovy = 45.0f;  // Field-of-view in Y direction angle (in degrees)
GLfloat  aspect;       // Viewport aspect ratio
GLfloat  zNear = 0.5f, zFar = 25.0f;

GLfloat angle = 0.0; // rotation angle in degrees
vec4 init_eye(7.0f, 3.0f, -10.0f, 1.0f); // initial viewer position
vec4 eye = init_eye;               // current viewer position

/* Sphere Information */
int sphere_vertices;
point4* sphere_points;
vec3* sphere_normals;
vec3* sphere_true_normals; 
color4 sphere_shadow_color(0.25, 0.25, 0.25, 0.65);

/* Floor Information */
const int floor_NumVertices = 6;
point4 floor_points[floor_NumVertices]; 
vec3   floor_normals[floor_NumVertices]; 
vec2   floor_texCoord[floor_NumVertices];

/* Axis Information */
const int axis_vertices = 2; 
point4 x_axis_points[axis_vertices];
color4 x_axis_color(1.0, 0.0, 0.0, 1.0);

point4 y_axis_points[axis_vertices];
color4 y_axis_color(1.0, 0.0, 1.0, 1.0);

point4 z_axis_points[axis_vertices];
color4 z_axis_color(0.0, 0.0, 1.0, 1.0);

vec3 axis_normals(1);

/* Fireworks Information */
const int firework_vertices = 300;
vec3   firework_velocity[firework_vertices];
color4 firework_color[firework_vertices];
float t = 0.0f;
float tsub = 0.0f;
float tmax = 10000.0f;

/* Positions of points A,B,C in world view */
point4 a_vec(3.0f, 1.0f, 5.0f, 1.0f);      
point4 b_vec(-2.0f, 1.0f, -2.5f, 1.0f);   
point4 c_vec(2.0f, 1.0f, -4.0f, 1.0f);   

/* Y Vector */
point4 y_vec(0.0f, 1.0f, 0.0f, 1.0f); 

/* Shadow projection matrix for sphere's shadow */
mat4 shadow_projection(vec4(12.0f, 14.0f, 0.0f, 0.0f),
    vec4(0.0f, 0.0f, 0.0f, 0.0f),
    vec4(0.0f, 3.0f, 12.0f, 0.0f),
    vec4(0.0f, -1.0f, 0.0f, 12.0f));

/*
Accumulated rotation matrix used for segment transition
Initally set as an identity matrix
*/
mat4 accumulated_rotation(1);

/*----- Shader Lighting Parameters -----*/
/* Global Lighting */
color4 global_ambient(1.0f, 1.0f, 1.0f, 1.0f);

/* Distant Directional Light */
vec4 dist_dir(0.1f, 0.0f, -1.0f, 0.0f); // In eye frame
color4 dist_amb(0.0f, 0.0f, 0.0f, 1.0f);
color4 dist_diff(0.8f, 0.8f, 0.8f, 1.0f);
color4 dist_spec(0.2f, 0.2f, 0.2f, 1.0f);

/* Positional Light (Point Source or Spot Light) */
point4 posl_pos(-14.0f, 12.0f, -3.0f, 1.0f);
vec4 posl_dir(point4(-6.0f, 0.0f, -4.5f, 1.0f) - posl_pos); // Spot Light
color4 posl_amb(0.0f, 0.0f, 0.0f, 1.0f);
color4 posl_diff(1.0f, 1.0f, 1.0f, 1.0f);
color4 posl_spec(1.0f, 1.0f, 1.0f, 1.0f);
float const_att = 2.0f;
float linear_att = 0.01f;
float quad_att = 0.001f;
float sl_exponent = 15.0f;     // Spot Light
float cutoff_angle = 20.0f; // Spot Light (In Degrees)

/* Sphere */
color4 sphere_amb(0.2f, 0.2f, 0.2f, 1.0f);
color4 sphere_diff(1.0f, 0.84f, 0.0f, 1.0f);
color4 sphere_spec(1.0f, 0.84f, 0.0f, 1.0f);
float  sphere_shininess = 125.0f;

/* Floor */
color4 floor_amb(0.2f, 0.2f, 0.2f, 1.0f);
color4 floor_diff(0.0f, 1.0f, 0.0f, 1.0f);
color4 floor_spec(0.0f, 0.0f, 0.0f, 1.0f);
float  floor_shininess = 1.0f;

/* Fog */
color4 fog_color(0.7, 0.7, 0.7, 0.5);
float fog_start = 0.0f;
float fog_end = 18.0f;
float fog_density = 0.09f;

/* Flags */
int seg_flag = 0;                 // 0: A to B, 1: B to C, 2: C to A
int begin_flag = 0;               // 0: Rolling has not begun, 1: Rolling has begun for the first time
int rolling_flag = 0;             // 0: Not Rolling, 1: Rolling
int shadow_flag = 0;              // 0: Shadow not displayed, 1: Shadow displayed
int shadow_blend_flag = 0;        // 0: Shadow Blending Disabled, 1: Shadow Blending Enabled
int sphere_fill_flag = 0;         // 0: Wire Frame Sphere & Shadow, 1: Filled Sphere & Shadow
int lighting_flag = 0;            // 0: Lighting disabled, 1: Lighting enabled
int light_source_flag = 0;        // 0: Point Source, 1: Spot Light
int shading_flag = 0;             // 0: Flat Shading; 1: Smooth Shading
int fog_flag = 0;                 // 0: Fog Off, 1: Linear Fog, 2: Exponential Fog, 3: Exponential Square Fog
int floor_tex_flag = 0;           // 0: Floor Texture Off, 1: Floor Texture On
int sphere_tex_flag = 0;          // 0: Sphere Texture Off, 1: Sphere Texture Contour Lines, 2: Sphere Texture Checkerboard
int sphere_tex_slant_flag = 0;    // 0: Vertical Texture, 1: Slanted Texture
int sphere_tex_space_flag = 0;    // 0: Texture in Object Space, 1: Texture in Eye Space
int sphere_lattice_flag = 0;      // 0: Lattice Effect Off, 1: Lattice Effect On
int sphere_lattice_comp_flag = 0; // 0: Upright Lattice, 1: Tilted Lattice
int fireworks_flag;               // 0: Fireworks Off, 1: Fireworks On

/* Global definitions for constants and global image arrays */
#define ImageWidth  64
#define ImageHeight 64

GLubyte Image[ImageHeight][ImageWidth][4];

#define	stripeImageWidth 32
GLubyte stripeImage[4 * stripeImageWidth];

GLuint texName;

/*************************************************************
void image_set_up(void):
  generate checkerboard and stripe images.

* Inside init(), call this function and set up texture objects
  for texture mapping.
  (init() is called from main() before calling glutMainLoop().)
***************************************************************/
void image_set_up(void)
{
    int i, j, c;

    /* --- Generate checkerboard image to the image array ---*/
    for (i = 0; i < ImageHeight; i++)
        for (j = 0; j < ImageWidth; j++)
        {
            c = (((i & 0x8) == 0) ^ ((j & 0x8) == 0));

            if (c == 1) /* white */
            {
                c = 255;
                Image[i][j][0] = (GLubyte)c;
                Image[i][j][1] = (GLubyte)c;
                Image[i][j][2] = (GLubyte)c;
            }
            else  /* green */
            {
                Image[i][j][0] = (GLubyte)0;
                Image[i][j][1] = (GLubyte)150;
                Image[i][j][2] = (GLubyte)0;
            }

            Image[i][j][3] = (GLubyte)255;
        }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /*--- Generate 1D stripe image to array stripeImage[] ---*/
    for (j = 0; j < stripeImageWidth; j++) {
        /* When j <= 4, the color is (255, 0, 0),   i.e., red stripe/line.
           When j > 4,  the color is (255, 255, 0), i.e., yellow remaining texture
         */
        stripeImage[4 * j] = (GLubyte)255;
        stripeImage[4 * j + 1] = (GLubyte)((j > 4) ? 255 : 0);
        stripeImage[4 * j + 2] = (GLubyte)0;
        stripeImage[4 * j + 3] = (GLubyte)255;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    /*----------- End 1D stripe image ----------------*/

    /*--- texture mapping set-up is to be done in
          init() (set up texture objects),
          display() (activate the texture object to be used, etc.)
          and in shaders.
     ---*/

} /* end function */

/* Generate 2 triangles: 6 vertices */
void floor()
{
    floor_points[0] = point4(5.0, 0.0, 8.0, 1.0);
    floor_points[1] = point4(-5.0, 0.0, -4.0, 1.0);
    floor_points[2] = point4(-5.0, 0.0, 8.0, 1.0);

    floor_points[3] = point4(-5.0, 0.0, -4.0, 1.0);
    floor_points[4] = point4(5.0, 0.0, 8.0, 1.0);
    floor_points[5] = point4(5.0, 0.0, -4.0, 1.0);

    floor_points[2] = point4(5.0, 0.0, -4.0, 1.0);
    floor_points[0] = point4(-5.0, 0.0, -4.0, 1.0);
    floor_points[1] = point4(5.0, 0.0, 8.0, 1.0);

    floor_points[3] = point4(5.0, 0.0, 8.0, 1.0);
    floor_points[4] = point4(-5.0, 0.0, -4.0, 1.0);
    floor_points[5] = point4(-5.0, 0.0, 8.0, 1.0);

    floor_texCoord[0] = vec2(0.0, 1.0);
    floor_texCoord[1] = vec2(1.0, 0.0);
    floor_texCoord[2] = vec2(1.0, 1.0);

    floor_texCoord[3] = vec2(1.0, 0.0);
    floor_texCoord[4] = vec2(0.0, 1.0);
    floor_texCoord[5] = vec2(0.0, 0.0);

    vec4 u = floor_points[1] - floor_points[0];
    vec4 v = floor_points[2] - floor_points[0];
    vec3 normal = normalize(cross(u, v));

    for (int i = 0; i < floor_NumVertices; i++)
        floor_normals[i] = normal;
}

/* Generate 3 Lines */
void axisLines()
{
    x_axis_points[0] = point4(0.0, 0.0, 2.0, 1.0);
    x_axis_points[1] = point4(10.0, 0.0, 2.0, 1.0);

    y_axis_points[0] = point4(0.0, 0.0, 2.0, 1.0);
    y_axis_points[1] = point4(0.0, 10.0, 2.0, 1.0);

    z_axis_points[0] = point4(0.0, 0.0, 2.0, 1.0);
    z_axis_points[1] = point4(0.0, 0.0, 10.0, 1.0);
}

/* Generate Fireworks w Random Color and Velocities */
void fireworks()
{
    for (int i = 0; i < firework_vertices; i++)
    {
        firework_velocity[i] = vec3(2.0 * (((rand() % 256) / 256.0) - 0.5),
                                    1.2 * 2.0 * ((rand() % 256) / 256.0),
                                    2.0 * (((rand() % 256) / 256.0) - 0.5));

        firework_color[i] = color4((rand() % 256) / 256.0,
                                 (rand() % 256) / 256.0,
                                 (rand() % 256) / 256.0,
                                 1.0);
    }
}

/* 
Read input from sphere file and generate corresponding 
triangles for sphere and its shadow 
*/
void file_in(void)
{
    float x, y, z;
    int numTriangles;
    char fileName[20];

    printf("What is the name of your input file?\n");
    scanf("%s", fileName);

    FILE* inputFile = fopen(fileName, "r");

    if (!inputFile)
        return;
    fscanf(inputFile, "%i", &numTriangles);
    sphere_vertices = numTriangles * 3;
    sphere_points = (point4*)malloc(sizeof(point4) * sphere_vertices);
    sphere_normals = (vec3*)malloc(sizeof(vec3) * sphere_vertices);
    sphere_true_normals = (vec3*)malloc(sizeof(vec3) * sphere_vertices);

    for (int i = 0; i < sphere_vertices; i++)
    {
        if (i % 3 == 0)
            fscanf(inputFile, "%*i");
        fscanf(inputFile, "%f %f %f", &x, &y, &z);
        sphere_points[i] = point4(x, y, z, 1.0);
    }

    for (int i = 0; i < sphere_vertices; i++)
    {
        vec3 normal;

        if (i % 3 == 0)
        {
            vec4 u = sphere_points[i + 1] - sphere_points[i];
            vec4 v = sphere_points[i + 2] - sphere_points[i];
            normal = normalize(cross(u, v));

            sphere_normals[i] = sphere_normals[i + 1] = sphere_normals[i + 2] = normal;
        }
        
        normal = normalize(-vec3(sphere_points[i].x, sphere_points[i].y, sphere_points[i].z));

        sphere_true_normals[i] = normal;
    }

    fclose(inputFile);
}

void init()
{
    glEnable(GL_DEPTH_TEST);

    image_set_up();

    /*--- Create and Initialize a 2D texture object for floor and sphere ---*/
    glGenTextures(1, &texName);      // Generate texture obj name(s)

    glActiveTexture(GL_TEXTURE0);  // Set the active texture unit to be 0 
    glBindTexture(GL_TEXTURE_2D, texName); // Bind the texture to this texture unit

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ImageWidth, ImageHeight,
        0, GL_RGBA, GL_UNSIGNED_BYTE, Image);

    /*--- Create and Initialize a 1D texture object for sphere ---*/
    glActiveTexture(GL_TEXTURE1);  // Set the active texture unit to be 1
    glBindTexture(GL_TEXTURE_1D, texName); // Bind the texture to this texture unit

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, stripeImageWidth,
        0, GL_RGBA, GL_UNSIGNED_BYTE, stripeImage);


    // Create and initialize a vertex buffer object for sphere, to be used in display()
    glGenBuffers(1, &sphere_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);
    glBufferData(GL_ARRAY_BUFFER,
        sizeof(point4) * sphere_vertices + sizeof(vec3) * sphere_vertices,
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        sizeof(point4) * sphere_vertices, sphere_points);
    glBufferSubData(GL_ARRAY_BUFFER,
        sizeof(point4) * sphere_vertices,
        sizeof(vec3) * sphere_vertices,
        sphere_normals);


    // Create and initialize a vertex buffer object for sphere's shadow, to be used in display()
    glGenBuffers(1, &sphere_shadow_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_shadow_buffer);
    glBufferData(GL_ARRAY_BUFFER,
        sizeof(point4) * sphere_vertices,
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        sizeof(point4) * sphere_vertices, sphere_points);

    floor();

    // Create and initialize a vertex buffer object for floor, to be used in display()
    glGenBuffers(1, &floor_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floor_points) + sizeof(floor_normals) + sizeof(floor_texCoord),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(floor_points), floor_points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points), sizeof(floor_normals),
        floor_normals);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points) + sizeof(floor_normals), sizeof(floor_texCoord),
        floor_texCoord);

    axisLines();

    // Create and initialize 3 vertex buffer objects for the axis lines, to be used in display()
    glGenBuffers(1, &x_axis_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, x_axis_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(x_axis_points),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(x_axis_points), x_axis_points);

    glGenBuffers(1, &y_axis_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, y_axis_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(y_axis_points),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(y_axis_points), y_axis_points);

    glGenBuffers(1, &z_axis_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, z_axis_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(z_axis_points),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(z_axis_points), z_axis_points);

    fireworks();

    // Create and initialize a vertex buffer object for floor, to be used in display()
    glGenBuffers(1, &firework_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, firework_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(firework_velocity) + sizeof(firework_color),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(firework_velocity),
        firework_velocity);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(firework_velocity), sizeof(firework_color),
        firework_color);

    // Load shaders and create a shader program (to be used in display()) - Non Fireworks
    program = InitShader("vshader53.glsl", "fshader53.glsl");

    // Load shaders and create a shader program (to be used in display()) - Fireworks
    programF = InitShader("fireworkvshader.glsl", "fireworkfshader.glsl");

    glClearColor(0.529, 0.807, 0.92, 0.0);
    glLineWidth(2.0);
    glPointSize(3.0);
}

/*
Set up lighting parameters that are uniform variables in shader for 
non-uniform colored objects.
*/
void setup_lighting_vars(mat4 mv, color4 o_amb, color4 o_diff, color4 o_spec, float o_shiny, int obj_flag)
{
    // Global Lighting
    glUniform4fv(glGetUniformLocation(program, "GlobalAmbProd"),
        1, global_ambient * o_amb);
    
    // Distant Directional Lighting
    glUniform4fv(glGetUniformLocation(program, "DistDirection"),
        1, dist_dir);
    glUniform4fv(glGetUniformLocation(program, "DistAmbProd"),
        1, dist_amb * o_amb);
    glUniform4fv(glGetUniformLocation(program, "DistDiffProd"),
        1, dist_diff * o_diff);
    glUniform4fv(glGetUniformLocation(program, "DistSpecProd"),
        1, dist_spec * o_spec);

    // Positional Lighting
    glUniform4fv(glGetUniformLocation(program, "PosPosition"),
        1, mv * posl_pos);
    glUniform4fv(glGetUniformLocation(program, "PosDirection"),
        1, mv * posl_dir);
    glUniform4fv(glGetUniformLocation(program, "PosAmbProd"),
        1, posl_amb * o_amb);
    glUniform4fv(glGetUniformLocation(program, "PosDiffProd"),
        1, posl_diff * o_diff);
    glUniform4fv(glGetUniformLocation(program, "PosSpecProd"),
        1, posl_spec * o_spec);
    glUniform1f(glGetUniformLocation(program, "ConstAtt"),
        const_att);
    glUniform1f(glGetUniformLocation(program, "LinearAtt"),
        linear_att);
    glUniform1f(glGetUniformLocation(program, "QuadAtt"),
        quad_att);
    glUniform1f(glGetUniformLocation(program, "SLExponent"),
        sl_exponent);
    glUniform1f(glGetUniformLocation(program, "CutoffAngle"),
        cutoff_angle * (M_PI / 180.0f));

    // Object Shinniness
    glUniform1f(glGetUniformLocation(program, "Shininess"),
        o_shiny);

    // Flags
    glUniform1i(glGetUniformLocation(program, "LightingFlag"),
        lighting_flag);
    glUniform1i(glGetUniformLocation(program, "LightSourceFlag"),
        light_source_flag);
    glUniform1i(glGetUniformLocation(program, "ConstColorFlag"),
        0);
    glUniform1i(glGetUniformLocation(program, "ObjFlag"),
        obj_flag);
}

/*
Set up lighting parameters that are uniform variables in shader for
uniform colored objects.
*/
void setup_color_lighting_vars(color4 const_color, int obj_flag)
{
    glUniform4fv(glGetUniformLocation(program, "ConstColor"),
        1, const_color);

    glUniform1i(glGetUniformLocation(program, "ConstColorFlag"),
        1);
    glUniform1i(glGetUniformLocation(program, "ObjFlag"),
        obj_flag);
}

/* Draw the object that is associated with the vertex buffer object "buffer" */
void drawObj(GLuint buffer, int num_vertices, int line_flag)
{
    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(0));

    GLuint vNormal = glGetAttribLocation(program, "vNormal");
    glEnableVertexAttribArray(vNormal);
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(sizeof(point4) * num_vertices));

    GLuint vTexCoord = glGetAttribLocation(program, "vTexCoord");
    glEnableVertexAttribArray(vTexCoord);
    glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(sizeof(point4) * num_vertices + sizeof(vec3) * num_vertices));

  /* Draw a sequence of geometric objs (triangles or lines) from the vertex buffer
     (using the attributes specified in each enabled vertex attribute array) */
    if(line_flag == 0)
        glDrawArrays(GL_TRIANGLES, 0, num_vertices);
    else
        glDrawArrays(GL_LINES, 0, num_vertices);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vNormal);
    glDisableVertexAttribArray(vTexCoord);
}

/* Draw the object that is associated with the vertex buffer object "firework_buffer" */
void drawFireworks(mat4 mv, mat4 p)
{
    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, firework_buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vVelocity = glGetAttribLocation(programF, "vVelocity");
    glEnableVertexAttribArray(vVelocity);
    glVertexAttribPointer(vVelocity, 3, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(0));

    GLuint vColor = glGetAttribLocation(programF, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(sizeof(vec3) * firework_vertices));

    glDrawArrays(GL_POINTS, 0, firework_vertices);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vVelocity);
    glDisableVertexAttribArray(vColor);
}

void display(void)
{
    GLuint  model_view;  // model-view matrix uniform shader variable location
    GLuint  projection;  // projection matrix uniform shader variable location

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program); // Use the shader program

    model_view = glGetUniformLocation(program, "model_view");
    projection = glGetUniformLocation(program, "projection");

    // Set the value of the fragment shader texture sampler variable
   //   ("texture_2D") to the appropriate texture unit. In this case,
   //   0, for GL_TEXTURE0 which was previously set in init() by calling
   //   glActiveTexture( GL_TEXTURE0 ).
    glUniform1i(glGetUniformLocation(program, "texture_2D"), 0);

    glUniform1i(glGetUniformLocation(program, "texture_1D"), 1);

    /* Set up Fog Uniform Variables in Shader */
    glUniform4fv(glGetUniformLocation(program, "FogColor"),
        1, fog_color);
    glUniform4fv(glGetUniformLocation(program, "Viewer"),
        1, eye);
    glUniform1f(glGetUniformLocation(program, "FogStart"),
        fog_start);
    glUniform1f(glGetUniformLocation(program, "FogEnd"),
        fog_end);
    glUniform1f(glGetUniformLocation(program, "FogDensity"),
        fog_density);
    glUniform1i(glGetUniformLocation(program, "FogFlag"),
        fog_flag);

    /* Set up Floor Texture Uniform Variables in Shader */
    glUniform1i(glGetUniformLocation(program, "FloorTexFlag"),
        floor_tex_flag);

    /* Set up Sphere Texture Uniform Variables in Shader */
    glUniform1i(glGetUniformLocation(program, "SphereFillFlag"),
        sphere_fill_flag);
    glUniform1i(glGetUniformLocation(program, "SphereTexFlag"),
        sphere_tex_flag);
    glUniform1i(glGetUniformLocation(program, "SphereTexSlantFlag"),
        sphere_tex_slant_flag);
    glUniform1i(glGetUniformLocation(program, "SphereTexSpaceFlag"),
        sphere_tex_space_flag);

    /* Set up Sphere Lattice Uniform Variables in Shader */
    glUniform1i(glGetUniformLocation(program, "SphereLatticeFlag"),
        sphere_lattice_flag);
    glUniform1i(glGetUniformLocation(program, "SphereLatticeCompFlag"),
        sphere_lattice_comp_flag);

    /*---  Set up and pass on Projection matrix to the shader ---*/
    mat4  p = Perspective(fovy, aspect, zNear, zFar);
    glUniformMatrix4fv(projection, 1, GL_TRUE, p);
    

    /*---  Set up and pass on Model-View matrix to the shader ---*/
    // eye is a global variable of vec4 set to init_eye and updated by keyboard()
    vec4 at(0.0, 0.0, 0.0, 1.0);
    vec4 up(0.0, 1.0, 0.0, 0.0);

    mat4  mv = LookAt(eye, at, up);

    vec4 seg_start_vec; // Starting position
    vec4 seg_end_vec; // Ending position
    vec4 seg_difference_vec; // Ending - Starting position
    vec4 rotation_vec; // Rotation axis
    vec4 translation_vec; // Translation vector
    mat4 rolling_matrix;
    float d; // Distance - Used in translation calculations

    if (seg_flag == 0)      // A to B
    {
        seg_start_vec = a_vec;
        seg_end_vec = b_vec;
        seg_difference_vec = b_vec - a_vec;
    }
    else if (seg_flag == 1) // B to C
    {
        seg_start_vec = b_vec;
        seg_end_vec = c_vec;
        seg_difference_vec = c_vec - b_vec;
    }
    else                    // C to A
    {
        seg_start_vec = c_vec;
        seg_end_vec = a_vec;
        seg_difference_vec = a_vec - c_vec;
    }

    d = angle * ((2 * M_PI) / 360); // Distance

    rotation_vec = cross(y_vec, seg_difference_vec);
    translation_vec = seg_start_vec + (seg_difference_vec * d * (1 / length(seg_difference_vec)));

    // Correct rolling with translation, rotation and smooth segment transition 
    rolling_matrix = Translate(translation_vec) * Rotate(angle, rotation_vec.x, rotation_vec.y, rotation_vec.z) * accumulated_rotation;

    /*----- Set Up the Model-View matrix for the sphere -----*/
    mv = mv * rolling_matrix;

    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);

    if (sphere_fill_flag == 1 && lighting_flag == 1)
        setup_lighting_vars(mv, sphere_amb, sphere_diff, sphere_spec, sphere_shininess, 1);
    else
        setup_color_lighting_vars(sphere_diff, 1);

    mat3 normal_matrix = NormalMatrix(mv, 0);
    glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"),
        1, GL_TRUE, normal_matrix);
    
    if (sphere_fill_flag == 0) // Wireframe sphere
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else              // Filled sphere
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    drawObj(sphere_buffer, sphere_vertices, 0);

    /* Update accumulated rotation if segment is finished */
    if (seg_flag == 0)      // A to B - Segment Finished
    {
        if (translation_vec.x <= seg_end_vec.x && translation_vec.z <= seg_end_vec.z)
        {
            seg_flag = (seg_flag + 1) % 3;
            accumulated_rotation = Rotate(angle, rotation_vec.x, rotation_vec.y, rotation_vec.z) * accumulated_rotation;
            angle = 0;
        }
    }
    else if (seg_flag == 1) // B to C - Segment Finished
    {
        if (translation_vec.x >= seg_end_vec.x && translation_vec.z <= seg_end_vec.z)
        {
            seg_flag = (seg_flag + 1) % 3;
            accumulated_rotation = Rotate(angle, rotation_vec.x, rotation_vec.y, rotation_vec.z) * accumulated_rotation;
            angle = 0;
        }
    }
    else                    // C to A - Segment Finished
    {
        if (translation_vec.x >= seg_end_vec.x && translation_vec.z >= seg_end_vec.z)
        {
            seg_flag = (seg_flag + 1) % 3;
            accumulated_rotation = Rotate(angle, rotation_vec.x, rotation_vec.y, rotation_vec.z) * accumulated_rotation;
            angle = 0;
        }
    }

    /* START OF CRITICAL SECTION*/
    /* Making shadow a decal on the floor */
    glDepthMask(GL_FALSE); // Disable Z-Buffer

    /*----- Set up the Mode-View matrix for the floor -----*/
    mv = LookAt(eye, at, up);

    normal_matrix = NormalMatrix(mv, 0);
    glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"),
        1, GL_TRUE, normal_matrix);

    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);

    if (lighting_flag == 1)
        setup_lighting_vars(mv, floor_amb, floor_diff, floor_spec, floor_shininess, 0);
    else
        setup_color_lighting_vars(floor_diff, 0);


    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    drawObj(floor_buffer, floor_NumVertices, 0);

    if(shadow_blend_flag == 0)
        glDepthMask(GL_TRUE); // Enable Z-Buffer

    if (shadow_flag == 1)
    {
        if (shadow_blend_flag == 1)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        /*----- Set Up the Model-View matrix for the sphere's shadow -----*/
        mv = LookAt(eye, at, up) * shadow_projection * rolling_matrix;

        glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);

        setup_color_lighting_vars(sphere_shadow_color, 2);

        if (sphere_fill_flag == 0) // Wireframe sphere shadow
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else              // Filled sphere shadow
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawObj(sphere_shadow_buffer, sphere_vertices, 0);

        if(shadow_blend_flag == 1)
            glDisable(GL_BLEND);
    }
    
    if(shadow_blend_flag == 1)
        glDepthMask(GL_TRUE); // Enable Z-Buffer

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Disable Frame Buffer

    /*----- Set up the Mode-View matrix for the floor -----*/
    mv = LookAt(eye, at, up);

    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
    
    if (lighting_flag == 1)
        setup_lighting_vars(mv, floor_amb, floor_diff, floor_spec, floor_shininess, 0);
    else
        setup_color_lighting_vars(floor_diff, 0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    drawObj(floor_buffer, floor_NumVertices, 0);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Enable Frame Buffer
    /* END OF CRITICAL SECTION */

    /*----- Set up the Model-View matrix for the axis lines -----*/
    mv = LookAt(eye, at, up) * Translate(0.0, 0.1, 0.0);

    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    setup_color_lighting_vars(x_axis_color, 3);
    drawObj(x_axis_buffer, axis_vertices, 1);
    c
            t);
        glUniformMatrix4fv(glGetUniformLocation(programF, "projection"),
            1, GL_TRUE, p);
        glUniformMatrix4fv(glGetUniformLocation(programF, "model_view"),
            1, GL_TRUE, mv);

        drawFireworks(mv, p);
    }

    glutSwapBuffers();
}

void idle(void)
{
    angle += 0.5f;

    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
    // Start and stop rolling
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN && begin_flag == 1)
    {
        rolling_flag = 1 - rolling_flag;

        if (rolling_flag == 1)
            glutIdleFunc(idle);
        else
            glutIdleFunc(NULL);
    }

}

void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        // Begin rolling and enable mouse functionality
    case 'b': case 'B':
        if (begin_flag == 0)
        {
            begin_flag = 1;
            rolling_flag = 1;
            glutIdleFunc(idle);
        }
        break;

    case 'X': eye[0] += 1.0; break;
    case 'x': eye[0] -= 1.0; break;
    case 'Y': eye[1] += 1.0; break;
    case 'y': eye[1] -= 1.0; break;
    case 'Z': eye[2] += 1.0; break;
    case 'z': eye[2] -= 1.0; break;

    case 'v': case'V':
        sphere_tex_slant_flag = 0;
        break;

    case 's': case'S':
        sphere_tex_slant_flag = 1;
        break;

    case 'o': case'O':
        sphere_tex_space_flag = 0;
        break;

    case 'e': case'E':
        sphere_tex_space_flag = 1;
        break;

    case 'l': case'L':
        sphere_lattice_flag = 1 - sphere_lattice_flag;

    case 'u': case'U':
        sphere_lattice_comp_flag = 0;
        break;

    case 't': case'T':
        sphere_lattice_comp_flag = 1;
        break;
    }

    glUniform4fv(glGetUniformLocation(program, "Viewer"),
        1, eye);

    glutPostRedisplay();
}

void action_menu(int id)
{
    switch (id)
    {
        // Reset to initial eye position
    case 1:
        eye = init_eye;
        break;

        // Display sphere as wire frame
    case 2:
        sphere_fill_flag = 0;
        break;

        // Exit
    case 3:
        exit(EXIT_SUCCESS);
        break;
    }

    glutPostRedisplay();
}

void shadow_menu(int id)
{
    switch (id)
    {
        // Show sphere's shadow
    case 1:
        shadow_flag = 1;
        break;

        // No shadow
    case 2:
        shadow_flag = 0;
        break;
    }

    glutPostRedisplay();
}

void shadow_blending_menu(int id)
{
    switch (id)
    {
        // Enable Shadow Blending
    case 1:
        shadow_blend_flag = 1;
        break;

        // Disable Shadow Blending
    case 2:
        shadow_blend_flag = 0;
        break;
    }

    glutPostRedisplay();
}

void lighting_menu(int id)
{
    switch (id)
    {
        // Lighting on
    case 1:
        lighting_flag = 1;
        break;

        // Lighting off
    case 2:
        lighting_flag = 0;
        break;
    }

    glutPostRedisplay();
}

void light_source_menu(int id)
{
    switch (id)
    {
        // Point Source
    case 1:
        light_source_flag = 0;
        break;

        // Spot Light
    case 2:
        light_source_flag = 1;
        break;
    }

    glutPostRedisplay();
}

void shading_menu(int id)
{
    switch (id)
    {
        // Flat Shading
    case 1:
        sphere_fill_flag = 1;
        shading_flag = 0;

        glGenBuffers(1, &sphere_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);
        glBufferData(GL_ARRAY_BUFFER,
            sizeof(point4) * sphere_vertices + sizeof(vec3) * sphere_vertices,
            NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
            sizeof(point4) * sphere_vertices, sphere_points);
        glBufferSubData(GL_ARRAY_BUFFER,
            sizeof(point4) * sphere_vertices,
            sizeof(vec3) * sphere_vertices,
            sphere_normals);

        break;

        // Smooth Shading
    case 2:
        sphere_fill_flag = 1;
        shading_flag = 1;

        glGenBuffers(1, &sphere_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);
        glBufferData(GL_ARRAY_BUFFER,
            sizeof(point4) * sphere_vertices + sizeof(vec3) * sphere_vertices,
            NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
            sizeof(point4) * sphere_vertices, sphere_points);
        glBufferSubData(GL_ARRAY_BUFFER,
            sizeof(point4) * sphere_vertices,
            sizeof(vec3) * sphere_vertices,
            sphere_true_normals);

        break;
    }

    glutPostRedisplay();
}

void fog_menu(int id)
{
    switch (id)
    {
        // No Fog
    case 1:
        fog_flag = 0;
        break;

        // Linear Fog
    case 2:
        fog_flag = 1;
        break;

        // Exponential Fog
    case 3:
        fog_flag = 2;
        break;

        // Exponential Square Fog
    case 4:
        fog_flag = 3;
        break;
    }

    glutPostRedisplay();
}

void floor_tex_menu(int id)
{
    switch (id)
    {
        // Floor Texture Mapping On
    case 1:
        floor_tex_flag = 1;
        break;

        // Floor Texture Mapping Off
    case 2:
        floor_tex_flag = 0;
        break;
    }

    glutPostRedisplay();
}

void sphere_tex_menu(int id)
{
    switch (id)
    {
        // Sphere Texture On - Contour Lines
    case 1:
        sphere_tex_flag = 1;
        break;

        // Sphere Texture On - Checkerboard
    case 2:
        sphere_tex_flag = 2;
        break;
  
        // Sphere Texture Mapping Off
    case 3:
        sphere_tex_flag = 0;
        break;
    }

    glutPostRedisplay();
}

void fireworks_menu(int id)
{
    switch (id)
    {
        // Fireworks On
    case 1:
        fireworks_flag = 1;
        tsub = (float)glutGet(GLUT_ELAPSED_TIME);
        break;

        // Fireworks Off
    case 2:
        fireworks_flag = 0;
        break;
    }

    glutPostRedisplay();
}

void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    aspect = (GLfloat)width / (GLfloat)height;
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    int action_menu_ID;
    int shadow_menu_ID;
    int shadow_blending_menu_ID;
    int lighting_menu_ID;
    int light_source_menu_ID;
    int shading_menu_ID;
    int fog_menu_ID;
    int floor_tex_menu_ID;
    int sphere_tex_menu_ID;
    int fireworks_menu_ID;

    file_in();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(640, 640);
    glutCreateWindow("Roll A Sphere");

    // Create shadow sub-menu
    shadow_menu_ID = glutCreateMenu(shadow_menu);
    glutSetMenuFont(shadow_menu_ID, GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Yes ", 1);
    glutAddMenuEntry(" No ", 2);

    // Create shadow blending sub-menu
    shadow_blending_menu_ID = glutCreateMenu(shadow_blending_menu);
    glutSetMenuFont(shadow_menu_ID, GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Yes ", 1);
    glutAddMenuEntry(" No ", 2);

    // Create lighting sub-menu
    lighting_menu_ID = glutCreateMenu(lighting_menu);
    glutSetMenuFont(lighting_menu_ID, GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Yes ", 1);
    glutAddMenuEntry(" No ", 2);

    // Create light source sub-menu
    light_source_menu_ID = glutCreateMenu(light_source_menu);
    glutSetMenuFont(lighting_menu_ID, GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Point Source ", 1);
    glutAddMenuEntry(" Spot Light ", 2);

    // Create shading sub-menu
    shading_menu_ID = glutCreateMenu(shading_menu);
    glutSetMenuFont(shading_menu_ID, GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Flat Shading ", 1);
    glutAddMenuEntry(" Smooth Shading ", 2);

    // Create fog sub-menu
    fog_menu_ID = glutCreateMenu(fog_menu);
    glutSetMenuFont(fog_menu_ID, GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" No Fog ", 1);
    glutAddMenuEntry(" Linear ", 2);
    glutAddMenuEntry(" Exponential ", 3);
    glutAddMenuEntry(" Exponential Square ", 4);

    // Create floor texture mapping sub-menu
    floor_tex_menu_ID = glutCreateMenu(floor_tex_menu);
    glutSetMenuFont(floor_tex_menu_ID, GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Yes ", 1);
    glutAddMenuEntry(" No ", 2);

    // Create sphere texture mapping sub-menu
    sphere_tex_menu_ID = glutCreateMenu(sphere_tex_menu);
    glutSetMenuFont(sphere_tex_menu_ID, GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Yes - Contour Lines ", 1);
    glutAddMenuEntry(" Yes - Checkerboard ", 2);
    glutAddMenuEntry(" No ", 3);

    // Create fireworks sub-menu
    fireworks_menu_ID = glutCreateMenu(fireworks_menu);
    glutSetMenuFont(fireworks_menu_ID, GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Yes ", 1);
    glutAddMenuEntry(" No ", 2);

    // Create action menu
    action_menu_ID = glutCreateMenu(action_menu);
    glutSetMenuFont(action_menu_ID, GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Default View Point ", 1);
    glutAddMenuEntry(" Wire Frame Sphere ", 2);
    glutAddSubMenu(" Shadow ", shadow_menu_ID);
    glutAddSubMenu(" Shadow Blending ", shadow_blending_menu_ID);
    glutAddSubMenu(" Lighting ", lighting_menu_ID);
    glutAddSubMenu(" Light Source ", light_source_menu_ID);
    glutAddSubMenu(" Shading ", shading_menu_ID);
    glutAddSubMenu(" Fog ", fog_menu_ID);
    glutAddSubMenu(" Texture Mapped Ground ", floor_tex_menu_ID);
    glutAddSubMenu(" Texture Mapped Sphere ", sphere_tex_menu_ID);
    glutAddSubMenu(" Firework ", fireworks_menu_ID);
    glutAddMenuEntry(" Quit ", 3);
    glutAttachMenu(GLUT_LEFT_BUTTON);

    /* Call glewInit() and error checking */
    int err = glewInit();
    if (GLEW_OK != err)
    {
        printf("Error: glewInit failed: %s\n", (char*)glewGetErrorString(err));
        exit(1);
    }

    // Get info of GPU and supported OpenGL version
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL version supported %s\n", glGetString(GL_VERSION));

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(NULL);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);

    init();
    glutMainLoop();

    return 0;
}