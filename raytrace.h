#ifndef RAY_TRACE_H
#define RAY_TRACE_H

#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#undef INFINITY
// use this instead, so there is no converting from float to int and errors and
// shit from that bullshit.
// yeah i cast it to a double hope this doesnt bite my ass
#define INFINITY (1<<30)// (double)LONG_MAX

typedef struct {
    unsigned int r;
    unsigned int g;
    unsigned int b;
} colour_t;

typedef struct {
    double x;
    double y;
    double z;
} iv3_t; // unsigned int vector 3

typedef struct {
    double x;
    double y;
} iv2_t; // unsigned int vector 2

typedef struct canvas_mt{
    size_t width;
    size_t height;
    size_t size;
    colour_t* pixels;
} canvas_t;

typedef struct {
    iv3_t center;
    double radius;
    colour_t colour;
    double specular;
} sphere_t;

typedef enum {
    ambient,
    point,
    directional,
} light_type;

typedef struct {
    light_type type;
    double intensity;
    union {
        iv3_t position;
        iv3_t direction;
    };
} light_t;

canvas_t init_canvas(size_t width, size_t height, colour_t pixels[width*height]);
iv3_t canvas_to_viewport(canvas_t* can, int x, int y);

iv2_t intersect_ray_sphere(iv3_t o, iv3_t D, sphere_t sphere);
colour_t trace_ray(iv3_t ray_origin, iv3_t d, double t_min, double t_max, sphere_t* objects, light_t* lights); 
double compute_lighting(iv3_t point, iv3_t normal, iv3_t V, int s, light_t* lights);

void put_pixel(canvas_t* can, int x, int y, colour_t col);
void render(canvas_t* canvas, const char* path);

colour_t clamp_colour(colour_t* col);
void debug_iv3(iv3_t a);
void error_out(const char* fmt, ...);

static inline colour_t scalar_mul_colour(colour_t a, double b);
static inline iv3_t    scalar_mul(iv3_t a, double b);
static inline iv3_t    scalar_div(iv3_t a, double b);
static inline iv3_t    iv3_sub(iv3_t a, iv3_t b);
static inline iv3_t    iv3_add(iv3_t a, iv3_t b);
static inline double   iv3_length(iv3_t vec);
static inline double   iv3_dot(iv3_t a, iv3_t b);
static inline double   iv3_cmp(iv3_t a, iv3_t b);
static inline double   colour_cmp(colour_t a, colour_t b);
// IMPORTANT: does not compare colours
static inline double   sphere_cmp(sphere_t a, sphere_t b);

#endif // RAY_TRACE_H
