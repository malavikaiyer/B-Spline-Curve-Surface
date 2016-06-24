#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#define MAX_CPTS  25
#define MAX_KNOT_VALUES MAX_CPTS+5
#define MAX_BPTS MAX_KNOT_VALUES*100

float vertex[MAX_BPTS][3], tri_normal[MAX_BPTS][3], vertex_normal[MAX_BPTS][3], triangle[MAX_BPTS][3];
int num_triangles = 0;
int num_vertices = -1;

int width = 500, height = 500;


static GLfloat theta[] = {0.0,0.0,0.0};


#define imageWidth 64
#define imageHeight 64

struct point{
    GLfloat x,y,z;
};


int ctrlPt_On = 1;
int selectPt = -1;
int BSpline_On = 1;
int ctrlPolygonOn = 1;
int movePointOn = -1;
int wireframe = -1;
int textureMapOn = -1;
int shade = -1;
int deletePointOn = -1;
GLfloat cpts[MAX_CPTS][3];
int ncpts = 0;
int numBpts = 0;
int cpolygonOn = 1;
float BSpline[MAX_BPTS][2];
float knot[MAX_KNOT_VALUES];

int numCurves = 0;
point triangleMesh[MAX_BPTS][20];

int selectedPoint = -1;
float curPos[3];
float lastPos[3] = {0.0F, 0.0F, 0.0F};

GLfloat mat_specular[]={1.0, 1.0, 1.0, 1.0};
GLfloat mat_diffuse[]={0.0, 0.7, 0.7, 1.0};
GLfloat mat_ambient[]={0.0, 0.2, 0.2, 1.0};
GLfloat mat_shininess[]={100.0};

FILE *jFile = NULL;
char *fileName = "sbspline.txt";
int buffer[100];
int ct = 0;

//Draws control points
void drawCtrlPoints()
{
    glPointSize(5.0);
    glBegin(GL_POINTS);
    for (int i = 0; i < ncpts; i++)
    {
        if(i == selectedPoint)
            glColor3f(0.0, 0.0, 1.0);
        else
            glColor3f(0.0, 0.0, 0.0);
        glVertex3fv(cpts[i]);
    }
    glEnd();
}

//Draws the control polygon
void drawCtrlPolygon()
{
    int i;
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_STRIP);
    for (i = 0; i < ncpts; i++)
        glVertex3fv(cpts[i]);
    glEnd();
}

//Implements the Cox de Boor Algorithm
float CoxdeBoor(int i, int p, float t)
{
    float	left, right;
    
    if (p==1) {
        if (knot[i] < knot[i+1] && knot[i] <= t && t < knot[i+1])
            return( 1.0 );
        else
            return( 0.0 );
    }
    else {
        if (knot[i+p-1] - knot[i] != 0.0)
            left = CoxdeBoor(i, p-1, t)*(t - knot[i])/
            (knot[i+p-1] - knot[i]);
        else
            left = 0.0;
        
        if (knot[i+p] - knot[i+1] != 0.0)
            right = CoxdeBoor(i+1, p-1, t)*(knot[i+p] - t)/
            (knot[i+p] - knot[i+1]);
        else
            right = 0.0;
        
        return( left + right );
    }
}


//Draws the B-Spline Curve
void drawBsplineCurve()
{
    int		i;
    int		m = ncpts - 1;
    int		num_knots;
    float	t, B0, B1, B2, B3, x, y;
    
    for (i=0; i<=3; i++)
        knot[i] = 0.0;
    
    for (i=4; i<=m; i++)
        knot[i] = i - 3.0;
    
    for (i=m+1;  i<=m+4; i++)
        knot[i] = m - 2.0;
    
    num_knots = m+4;
    numBpts = -1;
    
    
    
    for (i=3; i < num_knots-3; i++)
    {
        for (t = knot[i]; t < knot[i+1]; t += 0.2)
        {
            B0 = CoxdeBoor(i, 4, t);
            B1 = CoxdeBoor(i-1, 4, t);
            B2 = CoxdeBoor(i-2, 4, t);
            B3 = CoxdeBoor(i-3, 4, t);
            
            x = cpts[i][0] * B0 +
            cpts[i-1][0] * B1 +
            cpts[i-2][0] * B2 +
            cpts[i-3][0] * B3;
            
            y = cpts[i][1] * B0 +
            cpts[i-1][1] * B1 +
            cpts[i-2][1] * B2 +
            cpts[i-3][1] * B3;
            
            numBpts++;
            BSpline[ numBpts][0] = x;
            BSpline[ numBpts][1] = y;
        }
    }
    
    numBpts++;
    BSpline[numBpts][0] = cpts[ncpts-1][0];
    BSpline[numBpts][1] = cpts[ncpts-1][1];
    
    glColor3f(0.0, 0.0, 0.0);
    
    glBegin(GL_LINE_STRIP);
    for (i = 0; i <= numBpts; i++) {
        glVertex3f(BSpline[i][0],BSpline[i][1], 0.0 );
    }
    glEnd();
}

void Accumulate(int triangle_num, int vertex1, int vertex2, int vertex3)
{
    
    vertex_normal[vertex1][0] += tri_normal[triangle_num][0];
    vertex_normal[vertex1][1] += tri_normal[triangle_num][1];
    vertex_normal[vertex1][2] += tri_normal[triangle_num][2];
    
    vertex_normal[vertex2][0] += tri_normal[triangle_num][0];
    vertex_normal[vertex2][1] += tri_normal[triangle_num][1];
    vertex_normal[vertex2][2] += tri_normal[triangle_num][2];
    
    vertex_normal[vertex3][0] += tri_normal[triangle_num][0];
    vertex_normal[vertex3][1] += tri_normal[triangle_num][1];
    vertex_normal[vertex3][2] += tri_normal[triangle_num][2];
}



void Compute_Triangle_Normals()
{
    
    int counter_clock_wise = 1;
    
    int	i, vertex1, vertex2, vertex3;
    float	ax, ay, az, bx, by, bz, norm;
    
    for (i=0; i<=num_triangles; i++) {
        vertex1 = triangle[i][0];
        vertex2 = triangle[i][1];
        vertex3 = triangle[i][2];
        
        if (counter_clock_wise) {
            ax = vertex[vertex2][0] - vertex[vertex1][0];
            ay = vertex[vertex2][1] - vertex[vertex1][1];
            az = vertex[vertex2][2] - vertex[vertex1][2];
            
            bx = vertex[vertex3][0] - vertex[vertex1][0];
            by = vertex[vertex3][1] - vertex[vertex1][1];
            bz = vertex[vertex3][2] - vertex[vertex1][2];
        }
        else {
            ax = vertex[vertex3][0] - vertex[vertex1][0];
            ay = vertex[vertex3][1] - vertex[vertex1][1];
            az = vertex[vertex3][2] - vertex[vertex1][2];
            
            bx = vertex[vertex2][0] - vertex[vertex1][0];
            by = vertex[vertex2][1] - vertex[vertex1][1];
            bz = vertex[vertex2][2] - vertex[vertex1][2] ;
        }
        
        
        
        tri_normal[i][0] = ay*bz - by*az;
        tri_normal[i][1] = bx*az - ax*bz;
        tri_normal[i][2] = ax*by - bx*ay;
        
        norm = sqrt(tri_normal[i][0]*tri_normal[i][0] +
                    tri_normal[i][1]*tri_normal[i][1] +
                    tri_normal[i][2]*tri_normal[i][2]);
        
        if (norm != 0.0) {
            tri_normal[i][0] = tri_normal[i][0]/norm;
            tri_normal[i][1] = tri_normal[i][1]/norm;
            tri_normal[i][2] = tri_normal[i][2]/norm;
        }
    }
}


//Compute the surface normal at each vertex.
void Compute_Vertex_Normals()
{
    int     i;
    float	norm;
    
    for (i=0; i<= num_vertices; i++) {
        vertex_normal[i][0] = 0.0;
        vertex_normal[i][1] = 0.0;
        vertex_normal[i][2] = 0.0;
    }
    
    
    for (i=0; i<=num_triangles; i++)
        Accumulate(i, triangle[i][0], triangle[i][1], triangle[i][2]);
    
    for (i=0; i<=num_vertices; i++) {
        norm = sqrt(vertex_normal[i][0]*vertex_normal[i][0] +
                    vertex_normal[i][1]*vertex_normal[i][1] +
                    vertex_normal[i][2]*vertex_normal[i][2]);
        
        if (norm != 0.0) {
            vertex_normal[i][0] /= norm;
            vertex_normal[i][1] /= norm;
            vertex_normal[i][2] /= norm;
        }
    }
}


//Generates points for the B-Spline surface
void draw_BSpline_Surface()
{
    num_vertices = -1;
    num_triangles = 0;
    for(int i=0; i<=numBpts;i++)
    {
        vertex[++num_vertices][0] = BSpline[i][0];
        vertex[num_vertices][1] = BSpline[i][1];
        vertex[num_vertices][2] = 0;
    }
    int offset = num_vertices + 1;
    float theta_incr = 20;
    for(int theta_n = theta_incr; theta_n < 360 ; theta_n+=theta_incr)
    {
        
        for(int i=0; i<=numBpts; i++ )
        {
            vertex[++num_vertices][0] = BSpline[i][0] * cos(theta_n*3.14/180);
            vertex[num_vertices][1] = BSpline[i][1];
            vertex[num_vertices][2] = -BSpline[i][1] * sin(theta_n*3.14/180);
            
            if(i!=0)
            {
                num_triangles+=1;
                triangle[num_triangles][0] = num_vertices;
                triangle[num_triangles][1] = num_vertices-offset-1;
                triangle[num_triangles][2] = num_vertices-1;
                
                num_triangles+=1;
                triangle[num_triangles][0] = num_vertices;
                triangle[num_triangles][1] = num_vertices-offset;
                triangle[num_triangles][2] = num_vertices-offset-1;
                
            }
        }
        
    }
    
    int prev_vertex = num_vertices-offset+1;
    
    for(int i=0; i< numBpts; i++)
    {
        prev_vertex++;
        num_triangles+= 1;
        triangle[num_triangles][0] = i;
        triangle[num_triangles][1] = prev_vertex-1;
        triangle[num_triangles][2] = i-1;
        
        num_triangles+=1;
        triangle[num_triangles][0] = i;
        triangle[num_triangles][1] = prev_vertex;
        triangle[num_triangles][2] = prev_vertex-1;
    }
    
    Compute_Triangle_Normals();
    Compute_Vertex_Normals();
    
    
}

//Draws the B-Spline surface
void generate_mesh()
{
    int i,j;
    int v1, v2, v3;
    
    
    point normal;
    
    glColor3f(1.0, 3.0, 3.0);
    if(shade == -1)
        glBegin(GL_LINE_STRIP);
    else
        glBegin(GL_TRIANGLES);
    for(int i = 0; i<num_triangles; i++)
    {
        
        v1 = triangle[i][0];
        v2 = triangle[i][1];
        v3 = triangle[i][2];
        
        
        glColor3f(1.0, 3.0, 3.0);
        glTexCoord2f(0.0, 0.0);
        glNormal3fv(vertex_normal[v1]);
        glVertex3fv(vertex[v1]);
        
        glColor3f(1.0, 3.0, 3.0);
        glTexCoord2f(0.0, 0.3);
        glNormal3fv(vertex_normal[v2]);
        glVertex3fv(vertex[v2]);
        
        glColor3f(1.0, 3.0, 3.0);
        glTexCoord2f(0.3, 0.0);
        glNormal3fv(vertex_normal[v3]);
        glVertex3fv(vertex[v3]);
        
    }
    glEnd();
    
}




void mouseMotion(int x, int y)
{
    float  dx;
    
    float wx = (20.0 * x) / (float)(width - 1) - 10.0;
    float wy = (20.0 * (height - 1 - y)) / (float)(height - 1) - 10.0;
    lastPos[0] = curPos[0];
    lastPos[1] = curPos[1];
    lastPos[2] = curPos[2];
    
    curPos[0] = wx;
    curPos[1] = wy;
    curPos[2] = 0;
    
    if(selectPt == 1 && movePointOn == 1){
        cpts[selectedPoint][0] += (curPos[0] - lastPos[0]);
        cpts[selectedPoint][1] += (curPos[1] - lastPos[1]);
        cpts[selectedPoint][2] += (curPos[2] - lastPos[2]);
        glutPostRedisplay();
    }
}

static void mouse(int button, int state, int x, int y)
{
    float wx, wy;
    
    
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
        return;
    
    wx = (20.0 * x) / (float)(width - 1) - 10.0;
    wy = (20.0 * (height - 1 - y)) / (float)(height - 1) - 10.0;
    curPos[0] = wx;
    curPos[1] = wy;
    curPos[2] = 0;
    
    if(selectPt == 1){
        for(int i = 0; i < ncpts; i++){
            if(fabs(cpts[i][0] - wx) < 0.5 && fabs(cpts[i][1] - wy) < 0.5){
                selectedPoint = i;
            }
        }
        glutPostRedisplay();
    }
    if (ctrlPt_On == 1 && ncpts != MAX_CPTS ){
        
        cpts[ncpts][0] = wx;
        cpts[ncpts][1] = wy;
        cpts[ncpts][2] = 0.0;
        ncpts++;
        glutPostRedisplay();
    }
}





void lighting(){
    
    GLfloat lightpos[] = {5.0, 0.0, 0.0, 1.0};
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
    
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    
    glColorMaterial ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE ) ;
    glEnable ( GL_COLOR_MATERIAL ) ;
    
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
}

void display(void)
{
    int i;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glLoadIdentity();
    glRotatef(theta[0], 1.0, 0.0, 0.0);
    glRotatef(theta[1], 0.0, 1.0, 0.0);
    glRotatef(theta[2], 0.0, 0.0, 1.0);
    
    
    
    if (ctrlPolygonOn == 1)
        drawCtrlPolygon();
    
    if (BSpline_On == 1 && ncpts>3 )
        drawBsplineCurve();
    
    if(wireframe == 1)
    {
        draw_BSpline_Surface();
        generate_mesh();
    }
    
    
    drawCtrlPoints();
    
    
    glFlush();
    glutSwapBuffers();
}



void menu(int id)
{
    switch(id)
    {
        case 1:
            //Control point input
            ctrlPt_On = -ctrlPt_On;
            break;
        case 2:
            //Control polygon input
            ctrlPolygonOn = -ctrlPolygonOn;
            break;
        case 3:
            //B-Spline curve
            BSpline_On = -BSpline_On;
            break;
        case 4:
            //Selecting a point
            selectPt = -selectPt;
            ctrlPt_On = -1;
            break;
        case 5:
            //Deleting and moving points
            deletePointOn = -deletePointOn;
            if(selectPt == 1 && selectedPoint > -1)
            {
                for(int i = selectedPoint; i < ncpts - 1; i++){
                    cpts[i][0] = cpts[i+1][0];
                    cpts[i][1] = cpts[i+1][1];
                    cpts[i][2] = cpts[i+1][2];
                }
                selectedPoint = -1;
            }
            movePointOn = -movePointOn;
            break;
            
        case 6:
            //Saving the control points
            jFile = fopen(fileName, "w");
            if (jFile == NULL)
            {
                printf("Warning: Could not open %s\n", fileName);
            }
            else {
                for(int j=0;j<ncpts;j++)
                {
                    fprintf(jFile, "%f ", cpts[j][0]);
                    fprintf(jFile, "%f ", cpts[j][1]);
                    fprintf(jFile, "%f ", cpts[j][2]);
                    //fprintf(jFile, "\n");
                }
                fclose(jFile);
                printf("\nEvents saved in %s\n", fileName);
            }
            fclose(jFile);
            break;
            
        case 7:
            //Retrieving the control points
        {jFile = fopen(fileName, "r");
            float a,b,c;
            ncpts = -1;
            
            
            while((fscanf(jFile, "%f %f %f", &a, &b, &c)) != EOF)
            {
                ncpts++;
            }
            
            fscanf(jFile, "%f %f %f", &a, &b, &c);
            ncpts++;
            for(int i=0;i<ncpts;i++)
            {
                cpts[ncpts][0] = a;
                cpts[ncpts][1] = b;
                cpts[ncpts][2] = c;
                
            }
            
            drawCtrlPoints();
            fclose(jFile);
        }
            break;
        case 8:
            //Clear
            ncpts = 0;
            numBpts = 0;
            break;
        case 9:
            //Wireframe surface
            wireframe = -wireframe;
            ctrlPt_On = -1;
            break;
        case 10:
            //Shading of surface
            shade = -shade;
            break;
        case 11:
            //Texture mapping of surface
            textureMapOn = -textureMapOn;
            if(textureMapOn == 1)
            {
                glEnable(GL_TEXTURE_2D);
            }
            else
            {
                glDisable(GL_TEXTURE_2D);
            }
            break;
        case 12:
            exit(0);
        default:
            break;
    }
    glutPostRedisplay();
}


void reshape(int w, int h)
{
    width = w;
    height = h;
    
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-10.0, 10.0, -10.0, 10.0, -10.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);
}



void init()
{
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glOrtho(-10.0, 10.0, -10.0, 10.0, -10.0, 10.0);
}


void textureMap()
{
    
    GLubyte image[imageWidth][imageHeight][3];
    int i, j, r, c;
    for(i=0;i<imageHeight;i++)
    {
        for(j=0;j<imageWidth;j++)
        {
            c = ((((i&0x8)==0)^((j&0x8))==0))*255;
            image[i][j][0]= (GLubyte) c;
            image[i][j][1]= (GLubyte) c;
            image[i][j][2]= (GLubyte) c;
        }
    }
    
    glEnable(GL_DEPTH_TEST);
    
    
    glTexImage2D(GL_TEXTURE_2D,0,3,imageWidth,imageHeight,0,GL_RGB,GL_UNSIGNED_BYTE, image);
    
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_NEAREST);
    
    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
}

int main(int argc, char **argv)
{
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutCreateWindow("B-Spline Curve and Surface");
    
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    
    glutCreateMenu(menu);
    glutAddMenuEntry("Control point input", 1);
    glutAddMenuEntry("Control polygon", 2);
    glutAddMenuEntry("B-spline curve", 3);
    glutAddMenuEntry("Select control point", 4);
    glutAddMenuEntry("Delete or move control point", 5);
    glutAddMenuEntry("Save", 6);
    glutAddMenuEntry("Retrieve", 7);
    glutAddMenuEntry("Clear", 8);
    glutAddMenuEntry("Wireframe surface", 9);
    glutAddMenuEntry("Shaded surface", 10);
    glutAddMenuEntry("Textured surface", 11);
    glutAddMenuEntry("Quit", 12);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
    glutReshapeFunc(reshape);
    init();
    textureMap();
    lighting();
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glutMainLoop();
}