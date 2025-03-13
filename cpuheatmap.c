#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <sys/time.h>
#include <math.h>
#include <unistd.h>
#include <limits.h>
#include <GL/glut.h>

#include <glib-2.0/glib.h>
#include <glibtop.h>
#include <glibtop/cpu.h> 

#include "colors.h"

#define UPDATE_INTERVAL_MILLIS 33

#define DEFAULT_WINDOW_WIDTH 600
#define DEFAULT_WINDOW_HEIGHT 600

#define MAX_TILE_SPACING 10
#define TILE_SPACING_PERCENT 10

#define ATTACK_RATE 0.175
#define DECAY_RATE 0.184

glibtop *top;
glibtop_cpu *cpu_now;
glibtop_cpu *cpu_last;
glibtop_cpu *tmp;

int rows = 1;
int columns = 1;
int width = DEFAULT_WINDOW_WIDTH;
int height = DEFAULT_WINDOW_HEIGHT;

GLfloat tile_size = 1;
GLfloat tile_spacing = 0;
GLfloat padding_left = 0;
GLfloat padding_top = 0;

int cpu_count = 1;
double *cpu_load;

struct timespec time_now;
long long next_update = 0;

long long time_millis() {
  clock_gettime(CLOCK_MONOTONIC_RAW, &time_now);
  return (((long long) time_now.tv_sec) * 1000) + (time_now.tv_nsec / 1000000);
}

double compute_load(int cpu_num) {
    const guint64 total = cpu_now->xcpu_total[cpu_num] - cpu_last->xcpu_total[cpu_num];
    const guint64 idle = cpu_now->xcpu_idle[cpu_num] - cpu_last->xcpu_idle[cpu_num];
    return fmax(0, (double) (total - idle) / (double) total);
}

double smooth(double from, double to) {
   return from + ((to - from) * ((from < to) ? ATTACK_RATE : DECAY_RATE));
}

void update_cpu() {
    tmp = cpu_last;
    cpu_last = cpu_now;
    cpu_now = tmp;
    glibtop_get_cpu(cpu_now);
    for (int i=0; i < cpu_count; i++) {
        double curr = compute_load(i);
        double prev = cpu_load[i];
        cpu_load[i] = smooth(prev, curr);
    }
}

void compute_tile_size() {
    if (cpu_count <= 0 || width <= 0 || height <= 0) {
        tile_size = 0;
        rows = 0;
        columns = 0;
    }
    GLfloat max_side = 0;
    for (int rows = 1; rows <= cpu_count; rows++) {
        const GLfloat cols = (GLfloat) cpu_count / (GLfloat) rows;
        const GLfloat max_width_side = (GLfloat) width / cols;
        const GLfloat max_height_side = (GLfloat) height /(GLfloat) rows;
        const GLfloat possible_side = (max_width_side < max_height_side) ? max_width_side : max_height_side;
        if (possible_side > max_side) {
            max_side = possible_side;
        }
    }
    tile_size = max_side;
    columns = floor((GLfloat) width / tile_size);
    rows = floor((GLfloat) height / tile_size);
    tile_spacing = MIN(MAX_TILE_SPACING, tile_size * (TILE_SPACING_PERCENT / 100.0f));
    tile_size -= tile_spacing;
    const float extra_width = (float) width - (tile_size * (float) columns + tile_spacing * (float) (columns - 1));
    const float extra_height = (float) height - (tile_size * (float) rows + tile_spacing * (float) (rows - 1));
    padding_left = MAX(0, extra_width / 2.0f);
    padding_top = MAX(0, extra_height / 2.0f);
}

void on_resize(int w, int h) {
    width = w;
    height = h;
    compute_tile_size();
    width = w;
    height = h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -10, 10);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void set_color(int value) {
    if (value < 0 || value > 255) {
        glColor3f(0.0f, 0.0f, 0.0f);
        return;
    }
    glColor3f(COLORS[value][0], COLORS[value][1], COLORS[value][2]);
}

void draw_tiles() {
    glPushMatrix();
    glTranslatef(0, 0, 0.0f);

    GLfloat x = 0;
    GLfloat y = 0;
    int cpu = 0;

    glBegin(GL_QUADS);
    y = padding_top;
    for (int row=0; row<rows && cpu < cpu_count; row++) {
        x = padding_left;
        for (int col=0; col<columns && cpu < cpu_count; col++) {
            set_color((int) round(cpu_load[cpu] * 255));
            glVertex2f(x, y);
            glVertex2f(x, y + tile_size);
            glVertex2f(x + tile_size, y + tile_size);
            glVertex2f(x + tile_size, y);
            x += tile_size;
            if (col < columns - 1) {
                x += tile_spacing;
            }
            cpu++;

        }
        y += tile_size;
        if (row < rows - 1) {
            y += tile_spacing;
        }
    }
    glEnd();
}

void on_draw() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    draw_tiles();
    glutSwapBuffers();
}

void on_idle() {
    const long long now = time_millis();
    if (now < next_update) {
      usleep((next_update - now) * 1000);
      return;
    }
    next_update = now + UPDATE_INTERVAL_MILLIS;

    update_cpu();
    glutPostRedisplay();
 }

void init(int *argc, char **argv, int w, int h) {
    top = glibtop_init();
    
    cpu_count = top->ncpu + 1;
    printf("monitoring %d cpus... \n", cpu_count);

    compute_tile_size(width, height, cpu_count);

    cpu_load = calloc(cpu_count, sizeof(double));
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
