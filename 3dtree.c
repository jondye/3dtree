/* vi:set sw=2 ts=2 et: */

#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "read_png.h"

#ifdef WIN32
#include <windows.h>
#endif

#define WIN_X 800
#define WIN_Y 600

/* in case math.h dose not define PI */
#ifndef PI
#define PI 3.141593
#endif

/* Constants */
GLfloat container_color[] = {0.0, 0.2, 0.2, 1.0};
GLfloat water_color[]     = {0.0, 0.0, 0.8, 0.5};
GLfloat chute_color[]     = {0.5, 0.5, 0.5, 1.0};
GLfloat tree_color[]      = {0.6, 0.4, 0.0, 1.0}; 
GLfloat leaf_color[]      = {0.0, 0.5, 0.0, 1.0};
#define container_radius        5.0
#define container_height        10.0
#define door_height             2.5     /* height of door above ground */
#define chute_length            10.0    /* horizontal length of chute */
#define tray_size               8.0     /* length of the sides of the tray */
#define soil_subdivision_depth  5       /* level of subdivision used in soil */
#define soil_subdivision_drift  0.5     /* bumpiness of soil */
#define water_particles         20000   /* maximum number of water particles */
#define water_released          2000    /* max particles released per second */
#define water_particle_size     4.0     /* size of water particles */
#define water_particle_volume   0.0002  /* volume of each water particle */
#define water_start_velocity    1.0     /* pressure in tank */
#define wood_texture            0
#define soil_texture            1
#define gravity                 100.0
#define viewer_radius           30      /* distance of viewer from center */
#define camera_increment        (PI/100)/* distance camera moves per keypress */

/* Variables */
GLfloat container_water_level = container_height - 1.0;
GLfloat tray_water_level = 0.0;
GLfloat door_frame[3][3];
GLuint  textures[2];
GLfloat doory = 0.0;
GLfloat water[water_particles][7];
GLfloat tray_water_particle_volume;
int     active_particles = 0;
GLfloat viewer_position[3];
GLfloat viewer_y_angle = -1.0;
GLfloat viewer_x_angle = -0.5;

/* Display lists */
GLuint container;
GLuint tray;
GLuint chute;
GLuint soil;

/* Callbacks */
void display(void);
void reshape(int, int);
void special(int, int, int);
void timef(int);
void mouse(int, int, int, int);
void keyboard(unsigned char, int, int);
void idle(void);

/* Initialisation */
void init_textures(void);
void init_container(void);
void init_tray(void);
void init_chute(void);
void init_soil(void);
void init_water(void);
void init_tree(void);

/* Drawing functions */
void water_in_tank(void);
void water_in_tray(void);
void door(void);
void water_on_chute(void);
void tree(void);

/* helper functions */
void divide_triangle(int, GLfloat[3], GLfloat[3], GLfloat[3]);
GLfloat *cross_product(GLfloat[3], GLfloat[3], GLfloat[3]);
void normalise(GLfloat[3]);
GLfloat *difference(GLfloat[3], GLfloat[3], GLfloat[3]);
float gettime();
void calculate_water(void);
void new_particle(GLfloat[6]);
float randf(void);

int main(int argc, char *argv[]) {
  GLfloat ambient_light[] = {0.5, 0.5, 0.5, 1.0};

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(WIN_X, WIN_Y);
  glutCreateWindow("Assessment 2000");

  glClearColor(0.5, 0.5, 0.5, 1.0); /* Grey background */
  glShadeModel(GL_SMOOTH);
  glEnable(GL_DEPTH_TEST);
  /* enable lighting */
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient_light);
  glEnable(GL_LIGHTING);
  /* enable blending */
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  /* enable textures */
  glEnable(GL_TEXTURE_2D);

  /* initialise random numbers */
  srand((unsigned int) time(NULL));

  /* Initialise */
  init_textures();
  init_container();
  init_tray();
  init_chute();
  init_soil();
  init_water();
  init_tree();

  /* register callbacks */
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutSpecialFunc(special);
  glutMouseFunc(mouse);
  glutKeyboardFunc(keyboard);
  glutIdleFunc(idle);

  glutMainLoop();

  return 0;
}

void display() {
  GLfloat light_position[] = {20.0, 100.0, 50.0, 1.0};
  GLfloat light_color[] = {1.0, 1.0, 1.0, 1.0};

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* calculate viewpoint */
  viewer_position[0] = viewer_radius*cos(viewer_x_angle)*sin(viewer_y_angle);
  viewer_position[1] = -viewer_radius*sin(viewer_x_angle);
  viewer_position[2] = viewer_radius*cos(viewer_x_angle)*cos(viewer_y_angle);

  viewer_position[1] += (door_height+2)/2;
  viewer_position[2] += door_frame[0][2] + chute_length/2;

  /* Set up the viewpoint */
  glLoadIdentity();
  gluLookAt(viewer_position[0], viewer_position[1], viewer_position[2],
            0.0, (door_height+2.0)/2.0, door_frame[0][2]+chute_length/2,
            0.0, 1.0, 0.0);

  /* Place the lights */
  glLightfv(GL_LIGHT0, GL_AMBIENT_AND_DIFFUSE, light_color);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_color);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glEnable(GL_LIGHT0);

  glCallList(container);
  door();
  glCallList(tray);
  glCallList(chute);
  glCallList(soil);
  water_in_tray();
  water_on_chute();
  water_in_tank();
  tree();

  glFlush();
  glutSwapBuffers();
  return;
}

void reshape(int w, int h) {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, (float)w/(float)h, 2.0, 400.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glViewport(0, 0, w, h);
}

void init_textures() {
  GLbyte target_texture[128][128][3];
  unsigned int width, height;
  GLbyte * source_texture;

  glGenTextures(2, textures); /* create the texture objects */

  glBindTexture(GL_TEXTURE_2D, textures[wood_texture]);
  /* load and scale the texture */
  read_png("wood.png", &width, &height, &source_texture);
  if(0 != gluScaleImage(GL_RGB, width, height, GL_UNSIGNED_BYTE,
        source_texture, 128, 128, GL_UNSIGNED_BYTE, target_texture)) {
    fprintf(stderr, "ERROR: could not scale texture");
    exit(1);
  }
  free(source_texture);
  source_texture = NULL;

  /* apply the texture */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128,
               0, GL_RGB, GL_UNSIGNED_BYTE, target_texture);

  glBindTexture(GL_TEXTURE_2D, textures[soil_texture]);
  /* load and scale the texture */
  read_png("soil.png", &width, &height, &source_texture);
  if(0 != gluScaleImage(GL_RGB, width, height, GL_UNSIGNED_BYTE,
        source_texture, 128, 128, GL_UNSIGNED_BYTE, target_texture)) {
    fprintf(stderr, "ERROR: could not scale texture");
    exit(1);
  }
  free(source_texture);
  source_texture = NULL;

  /* apply the texture */
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE,
               target_texture);

  glBindTexture(GL_TEXTURE_2D, 0);
}

void door() {
  GLfloat black[] = {0.0, 0.0, 0.0, 1.0};
  GLfloat door[3][3];
  
  memcpy(door, door_frame, sizeof(GLfloat)*9);
  door[0][1] += doory;
  door[1][1] += doory;
  door[2][1] += doory;
  /* move the door forward slightly to stop it being obscured by the frame */
  door[0][2] += 0.05;
  door[1][2] += 0.05;
  door[2][2] += 0.05;

  /* Set the material to black */
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, black);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50);  
  
  /* draw the door */
  glBegin(GL_LINES);
    glVertex3fv(door[2]);
    glVertex3f(door[2][0], container_height, door[2][2]);
  glEnd();

  glLoadName(1);  /* for selection purposes */
  glBegin(GL_TRIANGLES);
    glNormal3f(0.0, 0.0, 1.0);
    glVertex3fv(door[0]);
    glVertex3fv(door[1]);
    glVertex3fv(door[2]);
  glEnd();

}

void water_on_chute() {
  register int i;

  /* Set the material to water */
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, water_color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, water_color);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100);

  glBegin(GL_POINTS);
    for(i = 0; i < water_particles; i++) {
      if(water[i][6]) {
        glVertex3fv(water[i]);
        /*glPushMatrix();
        glTranslatef(water[i][0], water[i][1], water[i][2]);
        glutSolidSphere(water_particle_size, 10, 10);
        glPopMatrix();*/
      }
    }
  glEnd();
}

void water_in_tray() {

  glPushMatrix();
    glTranslatef(0.0, 0.0, door_frame[0][2]+chute_length+tray_size/2);

    /* Set the material to water */
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, water_color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, water_color);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100);
    
    glBegin(GL_QUADS);
      glNormal3f(0.0, 1.0, 0.0);
      glVertex3f(tray_size/2, tray_water_level, tray_size/2);
      glVertex3f(tray_size/2, tray_water_level, -tray_size/2);
      glVertex3f(-tray_size/2, tray_water_level, -tray_size/2);
      glVertex3f(-tray_size/2, tray_water_level, tray_size/2);
    glEnd();
  glPopMatrix();
}

void water_in_tank() {
  float theta;
  int i, back, drawn;
  float PI_6, PI2_6;
  GLfloat x, z;
  GLfloat top[6][3], bottom[6][3], normals[6][3];

  PI_6 = PI/6;
  PI2_6 = 2*PI_6;

  glPushMatrix();
    glRotatef(-30.0, 0.0, 1.0, 0.0);

    /* Generate the vertices for the container */
    for(i = 0; i < 6; i++) {
      theta = i * PI2_6;
      
      /*
       * Normals for the planes
       * e.g. normals[0] is for the plane formed by 
       * top[0], top[1], bottom[0] and bottom[1]
       */
      normals[i][0] = sin(theta + PI_6);
      normals[i][1] = 0.0;
      normals[i][2] = cos(theta + PI_6);
      
      x = container_radius * sin(theta);
      z = container_radius * cos(theta);
      top[i][0] = x;
      top[i][1] = container_water_level;
      top[i][2] = z;
      bottom[i][0] = x;
      bottom[i][1] = 0.0;
      bottom[i][2] = z;
    }
    
    /* Set the material to water */
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, water_color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, water_color);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100);

    /* Draw the bottom */
    glBegin(GL_POLYGON);
      glNormal3f(0.0, -1.0, 0.0);
      for(i = 0; i < 6; i++)
        glVertex3fv(bottom[i]);
    glEnd();

    if(viewer_position[1] < container_water_level) {
      /* Draw the top */
      glBegin(GL_POLYGON);
        glNormal3f(0.0, 1.0, 0.0);
        for(i = 0; i < 6; i++)
          glVertex3fv(top[i]);
      glEnd();
    }

    /* Calculate which is the back of the container for blending purposes */
    if(viewer_position[2] == 0.0)
      theta = (viewer_position[0] < 0.0) ? -PI/2 : PI/2;
    else 
      theta = atan(viewer_position[0] / viewer_position[2]);
    if(viewer_position[2] < 0.0) theta += PI;
    theta -= PI_6;
    if(theta < 0.0) theta += 2*PI;

    back = (int) (theta/(PI2_6)) + 4;
    if(back > 5) back -= 6;
    
    /* Draw the walls */
    glBegin(GL_QUADS);
      i = (back == 0) ? 5 : back - 1;
      /* Draw the three back panels and the front right panel */
      for(drawn = 0; drawn < 4; drawn++, i++) {
        if(i > 5) i -= 6;
        glNormal3fv(normals[i]);
        glVertex3fv(bottom[i]);
        glVertex3fv(top[i]);
        if(i == 5) {
          glVertex3fv(top[0]);
          glVertex3fv(bottom[0]);
        } else {
          glVertex3fv(top[i+1]);
          glVertex3fv(bottom[i+1]);
        }
      }
      /* Draw the front left panel and then the front panel */
      if(i > 4) {
        i -= 5;
      } else {
        i += 1;
      }

      for(; drawn < 6; drawn++, i--) {
        if(i < 0) i += 6;
        glNormal3fv(normals[i]);
        glVertex3fv(bottom[i]);
        glVertex3fv(top[i]);
        if(i == 5) {
          glVertex3fv(top[0]);
          glVertex3fv(bottom[0]);
        } else {
          glVertex3fv(top[i+1]);
          glVertex3fv(bottom[i+1]);
        }
      }
    glEnd();

    if(viewer_position[1] >= container_water_level) {
      /* Draw the top */
      glBegin(GL_POLYGON);
        glNormal3f(0.0, 1.0, 0.0);
        for(i = 0; i < 6; i++)
          glVertex3fv(top[i]);
      glEnd();
    }
  
  glPopMatrix();

}

void init_water() {
  int i;

  tray_water_particle_volume = 1 * water_particle_volume / 
    (container_water_level - door_frame[0][1]);

  glPointSize(water_particle_size);
  for(i = 0; i < water_particles; i++)
    water[i][6] = 0;

  return;
}

void init_container() {
  float theta;
  int i;
  GLfloat x, z;
  GLfloat top[6][3], bottom[6][3];
  GLfloat doorz, door_half_width, door_frame_height;

  container = glGenLists(1);
  if(container != 0) {
    
    glNewList(container, GL_COMPILE);
      glPushMatrix();
        glRotatef(-30.0, 0.0, 1.0, 0.0);

        /* Generate the vertices for the container */
        for(i = 0; i < 6; i++) {
          theta = i * 2.0 * PI / 6.0;
          
          x = container_radius * sin(theta);
          z = container_radius * cos(theta);
          top[i][0] = x;
          top[i][1] = container_height;
          top[i][2] = z;
          bottom[i][0] = x;
          bottom[i][1] = 0.0;
          bottom[i][2] = z;
        }
        
        /* Set the material properties */
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
                     container_color);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, container_color);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 10);
        
        /* Draw the container */
        for(i = 0; i < 6; i++) {
          glBegin(GL_LINE_LOOP);
            glVertex3fv(bottom[i]);
            glVertex3fv(top[i]);
            if(i == 5) {
              glVertex3fv(top[0]);
              glVertex3fv(bottom[0]);
            } else {
              glVertex3fv(top[i+1]);
              glVertex3fv(bottom[i+1]);
            }
          glEnd();
        }
        
      glPopMatrix();

      /* calculate the door frame */
      doorz = container_radius * cos(PI/6);
      door_half_width = 0.5 * container_radius * sin(PI/6);
      door_frame[0][0] = -door_half_width;
      door_frame[0][1] = door_height;
      door_frame[0][2] = doorz;
      door_frame[1][0] = door_half_width;
      door_frame[1][1] = door_height;
      door_frame[1][2] = doorz;
      door_frame[2][0] = 0.0;
      door_frame_height = sqrt((2*door_half_width)*(2*door_half_width) - 
                               door_half_width*door_half_width);
      door_frame[2][1] = door_height + door_frame_height;
      door_frame[2][2] = doorz;

      /* draw the door frame */
      glBegin(GL_LINE_LOOP);
        glVertex3fv(door_frame[0]);
        glVertex3fv(door_frame[1]);
        glVertex3fv(door_frame[2]);
      glEnd();

    glEndList();
    
  } else {
    
    fprintf(stderr, "ERROR: Unable to allocate a display list for the container");
    exit(1);
  }
}

void init_tray() {
  int i;
  GLfloat tray_vertices[8][3] = {{tray_size/2, 0.0, tray_size/2},
                                 {-tray_size/2, 0.0, tray_size/2},
                                 {-tray_size/2, 0.0, -tray_size/2},
                                 {tray_size/2, 0.0, -tray_size/2},
                                 {tray_size/2, 2.0, tray_size/2},
                                 {-tray_size/2, 2.0, tray_size/2},
                                 {-tray_size/2, 2.0, -tray_size/2},
                                 {tray_size/2, 2.0, -tray_size/2}};
  GLfloat tray_normals[5][3] = {{0.0, 0.0, 1.0},
                                {-1.0, 0.0, 0.0},
                                {0.0, 0.0, -1.0},
                                {1.0, 0.0, 0.0},
                                {0.0, -1.0, 0.0}};
                                 
  tray = glGenLists(1);
  if(tray != 0) {
    glNewList(tray, GL_COMPILE);

      glPushMatrix();
      
        glTranslatef(0.0, 0.0, 
                     door_frame[0][2] + (chute_length) + tray_size/2);

        /* select the wood texture */
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glBindTexture(GL_TEXTURE_2D, textures[wood_texture]);

        /* draw the sides */
        glBegin(GL_QUADS);
          for(i = 0; i < 4; i++) {
            glNormal3fv(tray_normals[i]);
            glTexCoord2i(1,0); glVertex3fv(tray_vertices[i]);
            glTexCoord2i(1,1); glVertex3fv(tray_vertices[i+4]);
            if(i == 3) {
              glTexCoord2i(0,1); glVertex3fv(tray_vertices[4]);
              glTexCoord2i(0,0); glVertex3fv(tray_vertices[0]);
            } else {
              glTexCoord2i(0,1); glVertex3fv(tray_vertices[i+5]);
              glTexCoord2i(0,0); glVertex3fv(tray_vertices[i+1]);
            }
          }
        glEnd();


        /* draw the bottom */
        glBegin(GL_POLYGON);
          glNormal3fv(tray_normals[4]);
          glTexCoord2i(1,0); glVertex3fv(tray_vertices[0]);
          glTexCoord2i(0,0); glVertex3fv(tray_vertices[1]);
          glTexCoord2i(0,1); glVertex3fv(tray_vertices[2]);
          glTexCoord2i(1,1); glVertex3fv(tray_vertices[3]);
        glEnd();

        /* disable textures */
        glBindTexture(GL_TEXTURE_2D, 0);

      glPopMatrix();

    glEndList();
  } else {
    fprintf(stderr, "ERROR: unable to allocate a display list for the tray");
    exit(1);
  }
}

void init_chute() {
  GLfloat h, l, normal_length;

  chute = glGenLists(1);
  if(chute != 0) {
    glNewList(chute, GL_COMPILE);

      /* set the material properties */
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, chute_color);
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, chute_color);
      glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 80);

      /* draw the chute */
      h = (door_frame[0][1]-2.0);
      l = chute_length;
      normal_length = sqrt(h*h + l*l);

      glBegin(GL_QUADS);
        glNormal3f(0.0, l/normal_length, h/normal_length);
        glVertex3f(door_frame[0][0], door_frame[0][1], door_frame[0][2]);
        glVertex3f(door_frame[0][0], 2.0, door_frame[0][2] + chute_length);
        glVertex3f(door_frame[1][0], 2.0, door_frame[1][2] + chute_length);
        glVertex3f(door_frame[1][0], door_frame[1][1], door_frame[1][2]);

        glNormal3f(-1.0, 0.0, 0.0);
        glVertex3f(door_frame[0][0], door_frame[0][1]+0.5, door_frame[0][2]);
        glVertex3f(door_frame[0][0], door_frame[0][1], door_frame[0][2]);
        glVertex3f(door_frame[0][0], 2.0, door_frame[0][2] + chute_length);
        glVertex3f(door_frame[0][0], 2.5, door_frame[0][2] + chute_length);

        glNormal3f(1.0, 0.0, 0.0);
        glVertex3f(door_frame[1][0], door_frame[1][1]+0.5, door_frame[1][2]);
        glVertex3f(door_frame[1][0], door_frame[1][1], door_frame[1][2]);
        glVertex3f(door_frame[1][0], 2.0, door_frame[1][2] + chute_length);
        glVertex3f(door_frame[1][0], 2.5, door_frame[1][2] + chute_length);
      glEnd();
    glEndList();
  } else {
    fprintf(stderr, "ERROR: unable to allocate a display list for the chute");
    exit(1);
  }
}

void init_soil() {
  GLfloat p1[3], p2[3], p3[3], p4[3], centre[3] = {0.0, 1.0, 0.0};

  soil = glGenLists(1);
  if(soil != 0) {
    glNewList(soil, GL_COMPILE);
      
      glPushMatrix();
        glTranslatef(0.0, 0.0, door_frame[0][2]+chute_length+tray_size/2);
    
        p1[0] = p2[0] = p1[2] = p4[2] = -tray_size/2;
        p3[0] = p4[0] = p2[2] = p3[2] = tray_size/2;
        p1[1] = p2[1] = p3[1] = p4[1] = 1.0;
        
        /* select the soil texture */
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glBindTexture(GL_TEXTURE_2D, textures[soil_texture]);
        
        glBegin(GL_TRIANGLES);      
          divide_triangle(soil_subdivision_depth, p1, p2, centre);
          divide_triangle(soil_subdivision_depth, p2, p3, centre);
          divide_triangle(soil_subdivision_depth, p3, p4, centre);
          divide_triangle(soil_subdivision_depth, p4, p1, centre);
        glEnd();
        
        glBindTexture(GL_TEXTURE_2D, 0);
      glPopMatrix();
    glEndList();

  } else {
    fprintf(stderr, "ERROR: unable to allocate a display list for the chute");
    exit(1);
  }
}

void divide_triangle(int depth, GLfloat p1[3], GLfloat p2[3], GLfloat p3[3]) {
  GLfloat new_point[3];
  GLfloat normal[3], t1[3], t2[3];
  GLfloat ydrift;

  if(depth == 0) {
    /* calculate the normal */
    difference(p1,p2,t1);
    difference(p2,p3,t2);
    normalise(cross_product(t1,t2,normal));
    /* draw the triangle */
    glNormal3fv(normal);
    glTexCoord2f(0.0, 0.0); glVertex3fv(p1);
    glTexCoord2f(1.0, 0.0); glVertex3fv(p2);
    glTexCoord2f(0.5, 1.0); glVertex3fv(p3);
  } else {
    /* further sub-divide */
    new_point[0] = (p1[0] + p2[0] + p3[0])/3;
    ydrift = soil_subdivision_drift * 2*(randf()-0.5);
    new_point[1] = (p1[1] + p2[1] + p3[1])/3 + ydrift;
    new_point[2] = (p1[2] + p2[2] + p3[2])/3;
    divide_triangle(depth-1, p1, p2, new_point);
    divide_triangle(depth-1, p1, new_point, p3);
    divide_triangle(depth-1, new_point, p2, p3);
  }
}

void init_tree()
{

}

void keyboard(unsigned char key, int x, int y) {
  switch(key) {
  case 'o':
    /* open the door */
    glutTimerFunc(0, timef, 0);
    break;
  }
  return;
}

void special(int key, int x, int y) {
  switch(key) {
  case GLUT_KEY_UP:
    if(viewer_x_angle > -PI/2+camera_increment) 
      viewer_x_angle -= camera_increment;
    break;
  case GLUT_KEY_DOWN:
    if(viewer_x_angle < camera_increment) 
      viewer_x_angle += camera_increment;
    break;
  case GLUT_KEY_LEFT:
    viewer_y_angle -= camera_increment;
    if(viewer_y_angle < 0) viewer_y_angle += 2*PI;
    break;
  case GLUT_KEY_RIGHT:
    viewer_y_angle += camera_increment;
    if(viewer_y_angle > 2*PI) viewer_y_angle -= 2*PI;
    break;
  }
  glutPostRedisplay();
}

void timef(int timer)
{
  if(doory < 0.5) {
    doory += 0.05;
    glutTimerFunc(500.0, timef, 0);
    glutPostRedisplay();
  }
}

void idle() {
  static clock_t t_old=0;
  clock_t t_new, elapsed;

  t_new=clock();
  elapsed=t_new-t_old;
  t_old = t_new;
  printf("FPS: %.1f  \r", (float) CLOCKS_PER_SEC / (float) elapsed);

  if(doory > 0.0 && 
     (container_water_level > door_frame[0][1] || active_particles > 0)) {
    calculate_water();
    glutPostRedisplay();
  }
}

#define BUFSIZE 512

void mouse(int button, int state, int x, int y) {
  GLuint selectBuf[BUFSIZE];
  GLint viewport[4];

  if(button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;

  glGetIntegerv(GL_VIEWPORT, viewport);

  glSelectBuffer(BUFSIZE, selectBuf);
  glRenderMode(GL_SELECT);
  glInitNames();
  glPushName(0);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluPickMatrix((GLdouble) x, (GLdouble) (viewport[3] - y),
                2.0, 2.0, viewport);
  gluPerspective(45.0, (float)viewport[2]/(float)viewport[3], 2.0, 400.0);
  door();

  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glFlush();

  if(glRenderMode(GL_RENDER) == 1)
    glutTimerFunc(0, timef, 0);
  glutPostRedisplay();
  return;
}

GLfloat *cross_product(GLfloat m1[3], GLfloat m2[3], GLfloat result[3]) {
  result[0] = m1[1]*m2[2] - m2[1]*m1[2];
  result[1] = m2[0]*m1[2] - m1[0]*m2[2];
  result[2] = m1[0]*m2[1] - m2[0]*m1[1];
  return result;
}

void normalise(GLfloat v[3]) {
  GLfloat length;

  length = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
  v[0] = v[0]/length;
  v[1] = v[1]/length;
  v[2] = v[2]/length;
  return;
}

GLfloat *difference(GLfloat m1[3], GLfloat m2[3], GLfloat result[3]) {
  result[0] = m1[0] - m2[0];
  result[1] = m1[1] - m2[1];
  result[2] = m1[2] - m2[2];
  return result;
}

float gettime(){
  static clock_t t_old=0;
  clock_t t_new, elapsed;

  t_new=clock();

  elapsed=t_new-t_old;

  t_old = t_new;

  return (float) elapsed/(float) CLOCKS_PER_SEC;
}

float randf() {
  return (float)rand()/(float)RAND_MAX;
}

void new_particle(GLfloat particle[6]) {
  GLfloat h, l, mag_hl;
  GLfloat velocity;
  GLfloat wh;
  GLfloat dia;

  /* drop of chute */
  h = door_frame[0][1] - 2.0;
  /* horizontal length of chute */
  l = chute_length;
  /* length of chute */
  mag_hl = sqrt(h*h + l*l);
  /* distance from bottom of door frame to top of water in container */
  wh =  container_water_level-door_frame[0][1];
  /* diameter of particles */
  dia = water_particle_size/25.0;

  /* position */
  particle[0] = door_frame[0][0] + dia/2 + 
                randf()*(door_frame[1][0] - door_frame[0][0] - dia);
  particle[1] = door_frame[0][1] + dia + 
                randf()*( ((wh > 0.5) ? doory : wh) - dia);
  particle[2] = door_frame[0][2];

  /* velocity */
  velocity = (water_start_velocity + randf()*water_start_velocity)*wh;
  particle[3] = 0.0;
  particle[4] = -velocity*h/mag_hl;
  particle[5] = +velocity*l/mag_hl;

  /* activate particle */
  particle[6] = 1;

  /* decrease water level in container */
  container_water_level -= water_particle_volume;

  return;
}

void calculate_water(void) {
  register int i;
  float elapsed;
  GLfloat end_of_chute, y_acceleration;
  GLfloat acc, vert_acc, horiz_acc;
  GLfloat h, l, mag_hl;
  GLfloat wh, max_release;
  GLfloat dia;
  int released = 0;
  register int used = 0;

  /* compute values once */
  elapsed = gettime();
  end_of_chute = chute_length+door_frame[0][2];
  y_acceleration = gravity * elapsed;
  /* chute drop */
  h = door_frame[0][1] - 2.0;
  /* horizontal chute length */
  l = chute_length;
  /* chute length */
  mag_hl = sqrt(h*h + l*l);
  acc = gravity*h*cos(atan(l/h));
  vert_acc = elapsed*acc*h/mag_hl;
  horiz_acc = elapsed*acc*l/mag_hl;
  /* water height above bottom of door frame */
  wh =  container_water_level-door_frame[0][1];
  /* number of particles to release this time */
  max_release = elapsed * water_released/0.5 * ((wh > 0.5) ? doory : wh);
  /* particle diameter */
  dia = water_particle_size/25.0;

  for(i = 0; i < water_particles; i++) {
    if(water[i][6]) {  /* water particle is active */
      if(water[i][2] > end_of_chute) {
        /* fallen off the chute */
        /* gravity */
        water[i][4] -= y_acceleration;
        /* position */
        water[i][1] += water[i][4] * elapsed;
        water[i][2] += water[i][5] * elapsed;
        if(water[i][1] <= dia) {
          /* increase water level in tray */
          tray_water_level += tray_water_particle_volume;
          if(container_water_level > door_frame[0][1] && 
             released < max_release) {
            /* restart particle */
            new_particle(water[i]);
            released++;
          } else
            /* deactivate particle */
            water[i][6] = 0;
        }
      } else {
        /* on the chute */
        water[i][2] += water[i][5] * elapsed;
        water[i][1] += water[i][4] * elapsed;
        water[i][5] += horiz_acc;
        water[i][4] -= vert_acc;
      }
      used++;
    } else {
      if(container_water_level > door_frame[0][1] && 
         released < max_release) {
        /* release more water */
        new_particle(water[i]);
        released++;
      }
    }
  }
  if(used == water_particles)
    fprintf(stderr, "WARNING: Maximum number of particle reached\n");
  active_particles = used;
  return;
}

void leaf()
{
  /* draw a leaf */
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, leaf_color);
  glBegin(GL_TRIANGLES);
    glVertex3f(0.0, 0.2, 0.0);
    glVertex3f(0.2, 0.0, 0.0);
    glVertex3f(-0.2, 0.0, 0.0);
  glEnd();
}

void branch(float size, float branch_trigger)
{
  float const branch_length = 0.5 * size;

  /* draw a branch */
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, tree_color);
  glBegin(GL_LINES);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, branch_length, 0.0);
  glEnd();
  glTranslatef(0.0, branch_length, 0.0);

  if(size < branch_trigger) {
    leaf();
  } else {
    /* draw more branches */
    float const smaller_size = size - branch_length;
    float const smaller_branch_trigger = branch_trigger * 0.75;
    glPushMatrix();
      glRotatef(0.0, 0.0, 1.0, 0.0);
      glRotatef(25.0, 0.0, 0.0, 1.0);
      branch(smaller_size * 0.9, smaller_branch_trigger * 0.9);
    glPopMatrix();
    glPushMatrix();
      glRotatef(72.0, 0.0, 1.0, 0.0);
      glRotatef(30.0, 0.0, 0.0, 1.0);
      branch(smaller_size, smaller_branch_trigger * 0.9);
    glPopMatrix();
    glPushMatrix();
      glRotatef(144.0, 0.0, 1.0, 0.0);
      glRotatef(25.0, 0.0, 0.0, 1.0);
      branch(smaller_size * 0.9, smaller_branch_trigger);
    glPopMatrix();
    glPushMatrix();
      glRotatef(216.0, 0.0, 1.0, 0.0);
      glRotatef(30.0, 0.0, 0.0, 1.0);
      branch(smaller_size * 1.1, smaller_branch_trigger * 1.1);
    glPopMatrix();
    glPushMatrix();
      glRotatef(288.0, 0.0, 1.0, 0.0);
      glRotatef(35.0, 0.0, 0.0, 1.0);
      branch(smaller_size, smaller_branch_trigger * 1.1);
    glPopMatrix();
  }
}

void tree()
{
  glPushMatrix();
    /* Place the tree in the center of the tray */
    glTranslatef(0.0, 0.0, door_frame[0][2] + (chute_length) + tray_size/2);

    branch(tray_water_level * 10.0, 2.0);
  glPopMatrix();
}

