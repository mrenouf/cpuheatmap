#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <sys/time.h>
#include <math.h>
#include <unistd.h>

#include <GL/glut.h>

#include <glib-2.0/glib.h>
#include <glibtop.h>
#include <glibtop/cpu.h> 

#include "colors.h"

#define UPDATE_INTERVAL_MILLIS 33

#define TILE_SIZE 150
#define TILE_SPACING 10

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 1000

#define ATTACK_RATE 0.175
#define DECAY_RATE 0.184

glibtop *top;

glibtop_cpu *cpu_now;
glibtop_cpu *cpu_last;
glibtop_cpu *tmp;

int rows = 1;
int columns = 1;
int width = 1;
int height = 1;

int cpu_count = 1;
float *cpu_load;

struct timespec time_now;
long long next_update = 0;

long long time_millis() {
  clock_gettime(CLOCK_MONOTONIC_RAW, &time_now);
  return (((long long) time_now.tv_sec) * 1000) + (time_now.tv_nsec / 1000000);
}

float compute_load(int cpu_num) {
    long total = cpu_now->xcpu_total[cpu_num] - cpu_last->xcpu_total[cpu_num];
    long idle = cpu_now->xcpu_idle[cpu_num] - cpu_last->xcpu_idle[cpu_num];
    return fmax(0, (total - idle) / (float) total);
}

float smooth(float from, float to) {
   return from + ((to - from) * ((from < to) ? ATTACK_RATE : DECAY_RATE));
}

void update_cpu() {
    tmp = cpu_last;
    cpu_last = cpu_now;
    cpu_now = tmp;
    glibtop_get_cpu(cpu_now);
    for (int i=0; i < cpu_count; i++) {
        float curr = compute_load(i);
        float prev = cpu_load[i];
        cpu_load[i] = smooth(prev, curr);
    }
}

void on_resize(int w, int h) {
    printf("on_resize: w=%d, h=%d\n", w, h);
    columns = floor((w - TILE_SPACING) / (TILE_SIZE + TILE_SPACING));
    if (columns > 0) {
       rows = ceil(cpu_count / (float) columns);
    } else {
       columns = 1;
       rows = 1;
    }
    printf("%d row, %d col\n", rows, columns);
    width = w;
    height = h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -10, 10);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void set_color(int value) {
    if (value < 0 || value > 255) {
        glColor3f(0.0, 0.0, 0.0);
        return;
    }
    glColor3f(COLORS[value][0], COLORS[value][1], COLORS[value][2]);
}

void draw_tiles() {
    glPushMatrix();
    glTranslatef(0, 0, 0.0f);

    int x = 0;
    int y = 0;
    int cpu = 0;

    glBegin(GL_QUADS);

    for (int row=0; row<rows && cpu < cpu_count; row++) {
        x = TILE_SPACING;
        y += TILE_SPACING;

        for (int col=0; col<columns && cpu < cpu_count; col++) {
            set_color(cpu_load[cpu] * 255);
            glVertex2f(x, y);
            glVertex2f(x, y + TILE_SIZE);
            glVertex2f(x + TILE_SIZE, y + TILE_SIZE);
            glVertex2f(x + TILE_SIZE, y);      
            x += TILE_SIZE + TILE_SPACING;
            cpu++;
        }
        y += TILE_SIZE;
    }
    glEnd();
}

void on_draw() {
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    draw_tiles();
    glutSwapBuffers();
}

void on_idle() {
    long long now = time_millis();
    if (now < next_update) {
      usleep((next_update - now) * 1000);
      return;
    }
    next_update = now + UPDATE_INTERVAL_MILLIS;

    update_cpu();
    glutPostRedisplay();
 }

void init_size(int cpu_count) {
    float sr = sqrt(cpu_count);
    columns = ceil(sr);
    rows = floor(cpu_count / floor(sr));
    width = (columns * TILE_SIZE) + ((columns + 1) * TILE_SPACING);
    height = (rows * TILE_SIZE) + ((rows + 1) * TILE_SPACING); 
}

void init(int *argc, char **argv, int w, int h) {
    top = glibtop_init();
    
    cpu_count = top->ncpu + 1;
    printf("%d cpus online\n", cpu_count);
    init_size(cpu_count);
        
    cpu_load = calloc(cpu_count, sizeof(float));
    cpu_now = (glibtop_cpu*) calloc(1, sizeof(glibtop_cpu));
    cpu_last = (glibtop_cpu*) calloc(1, sizeof(glibtop_cpu));
    glibtop_get_cpu(cpu_now);
    glibtop_get_cpu(cpu_last);

    glutInit(argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("cpuheatmap");

    glutDisplayFunc(on_draw);
    glutReshapeFunc(on_resize);
    glutIdleFunc(on_idle);
}

void cleanup() {
    free(cpu_load);
    free(cpu_now);
    free(cpu_last);
    glibtop_close();
}

int main(int argc, char **argv) {
    init(&argc, argv, width, height);

    glutMainLoop();

    cleanup();
    return 0;
}
