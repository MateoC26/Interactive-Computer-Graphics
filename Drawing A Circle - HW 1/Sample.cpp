#include <stdio.h>
#include <math.h>

#ifdef __APPLE__  // include Mac OS X verions of headers
#include <GLUT/glut.h>
#else // non-Mac OS X operating systems
#include <GL/glut.h>
#endif

#define XOFF          50
#define YOFF          50
#define WINDOW_WIDTH  600
#define WINDOW_HEIGHT 600

void display(void);
void myinit(void);
void idle(void);
void draw_circle(int x, int y, int r);
int find_max_abs_coord(void);

/* Function to handle file input; modification may be needed */
void file_in(void);

/* Global Variabales*/
/* Manual Input x, y, r */
int in_x, in_y, in_r;

/* Input File Variables */
int num_circles;
int* circle_array;

/* Choice made by user */
int choice;

/* Animation Variables */
float frame_count = 0;
int max_frame = 144;

/*-----------------
The main function
------------------*/
int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    /* Use both double buffering and Z buffer */
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    /* Option selection */
    printf("Please select an option:\n1. Manually enter circle coordinates\n2. Use an input file\n3. Use an input file with animation\n");
    scanf("%i", &choice);

    if (choice == 1)
    {
        printf("\nInput 3 integers seperated by a space:\n");

        scanf("%i", &in_x);
        scanf("%i", &in_y);
        scanf("%i", &in_r);
    }
    else
    {
        /* Function call to handle file input here */
        file_in();
    }

    glutInitWindowPosition(XOFF, YOFF);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Assignment 1");
    glutDisplayFunc(display);
    glutIdleFunc(idle);

    myinit();
    glutMainLoop();

    return 0;
}

/*----------
file_in(): file input function. Modify here.
------------*/
void file_in(void)
{
    int num;
    char fileName[20];

    printf("\nWhat is the name of your file? (ex. fileName.txt) MAX 20 CHARS\n");

    scanf("%s", fileName);

    FILE* input_file;
    input_file = fopen(fileName, "r");

    if (!input_file)
        return;
    
    /* Obtain number of circles from input file and allocate memory for them */
    fscanf(input_file, "%i", &num_circles);
    circle_array = (int*)malloc(sizeof(int) * (num_circles * 3));

    /* Scan in circle info to array */
    for (int i = 0; i < num_circles * 3; i++)
    {
        fscanf(input_file, "%i", &num);
        circle_array[i] = num;
    }

    /* Modify the circle info in the array */
    int mac = find_max_abs_coord();
    for (int i = 0; i < num_circles; i++)
    {
        int x = circle_array[i * 3];
        int y = circle_array[(i * 3) + 1];
        int r = circle_array[(i * 3) + 2];

        /* Scale if necessary */
        if (mac > WINDOW_WIDTH / 2)
        {
            x = (x * WINDOW_WIDTH / 2) / mac;
            y = (y * WINDOW_HEIGHT / 2) / mac;
            r = (r * WINDOW_WIDTH / 2) / mac;
        }

        /* Translate world coords to screen coords */
        x += WINDOW_WIDTH / 2;
        y += WINDOW_HEIGHT / 2;

        circle_array[i * 3] = x;
        circle_array[(i * 3) + 1] = y;
        circle_array[(i * 3) + 2] = r;
    }

    fclose(input_file);
}

/*---------------------------------------------------------------------
display(): This function is called once for _every_ frame.
---------------------------------------------------------------------*/
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glColor3f(1.0f, 0.84f, 0.0f);         /* draw in golden yellow */
    glPointSize(1.0);                     /* size of each point */

    glBegin(GL_POINTS);

    //glVertex2i(300, 300);               /* draw a vertex here */

    if(choice == 1)
        draw_circle(in_x, in_y, in_r);
    else
    {
        for (int i = 0; i < num_circles; i++)
        {
            int x = circle_array[i * 3];
            int y = circle_array[(i * 3) + 1];
            int r = circle_array[(i * 3) + 2];

            /* Animation modification */
            if (choice == 3)
                r = (r * frame_count) / max_frame;

            draw_circle(x, y, r);
        }
    }

    glEnd();

    glFlush();                            /* render graphics */

    glutSwapBuffers();                    /* swap buffers */
}

/*---------------------------------------------------------------------
myinit(): Set up attributes and viewing
---------------------------------------------------------------------*/
void myinit()
{
    glClearColor(0.0f, 0.0f, 0.92f, 0.0f);    /* blue background*/

    /* set up viewing */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
}


/*---------------------------------------------------------------------
idle(): Automatically called when nothing occurs
---------------------------------------------------------------------*/
void idle()
{
    /* Update variables used for animation */
    if (choice == 3)
    {
        frame_count += 0.05df;

        if (frame_count >= max_frame)
            frame_count = 0;
    }

    glutPostRedisplay();
}

/*------------------------------------------------------------------------------
draw_circle(int x, int y, int r): Draw a circle centered at (x,y) with radius r
--------------------------------------------------------------------------------*/
void draw_circle(int x, int y, int r)
{
    /* Use(0, r) and translate later for actual x and y */
    int x_curr = 0;
    int y_curr = r;

    /* Decision Variable */
    int d_var = INT_MIN;

    /* Loop over(x, y) only */
    while (y_curr > x_curr)
    {
        if(d_var == INT_MIN) // Decision variable not set yet 
            d_var = x_curr ^ 2 + 2 * x_curr + y_curr ^ 2 - y_curr + 1 - r ^ 2; // dstart
        else                 // Obtaining dnew from dold
        {
            if (d_var < 0)   // Midpoint inside circle (Choose E)
            {
                d_var = d_var + (2 * x_curr) + 3;
                x_curr++;
            }
            else            // Midpoint outside of circle (Choose SE)
            {
                d_var = d_var + (2 * x_curr) - (2 * y_curr) + 5;
                x_curr++;
                y_curr--;
            }
        }
            
        /*
        Draw points symmetrically (Clockwise)
        Points are translated by x and y to fit circle
        */
        glVertex2i(x_curr + x, y_curr + y);   // (x,y)
        glVertex2i(y_curr + x, x_curr + y);   // (y,x)
        glVertex2i(y_curr + x, -x_curr + y);  // (y,-x)
        glVertex2i(x_curr + x, -y_curr + y);  // (x,-y)
        glVertex2i(-x_curr + x, -y_curr + y); // (-x,-y)
        glVertex2i(-y_curr + x, -x_curr + y); // (-y,-x)
        glVertex2i(-y_curr + x, x_curr + y);  // (-y,x)
        glVertex2i(-x_curr + x, y_curr + y);  // (-x,y)
    }
}

/*------------------------------------------------------------------------------
find_max_abs_coord(void): Return the max absolute coordinate
--------------------------------------------------------------------------------*/
int find_max_abs_coord(void)
{
    int max_x = INT_MIN;
    int max_y = INT_MIN;

    int curr_max_x;
    int curr_max_y;
   
    int x, y, r;

    /* Loop over every circle to find max x or y coordinate outside of screen */
    for (int i = 0; i < num_circles; i++)
    {
        x = circle_array[i * 3];
        y = circle_array[(i * 3) + 1];
        r = circle_array[(i * 3) + 2];

        curr_max_x = fmax(x + r, abs(x - r));
        curr_max_y = fmax(y + r, abs(y - r));

        if (curr_max_x > max_x)
            max_x = curr_max_x;

        if (curr_max_y > max_y)
            max_y = curr_max_y;
    }

    return (int)fmax(max_x, max_y);
}