#include "raytrace.h"

#define BACKGROUND_COLOUR (colour_t){.r = 255, .g = 255, .b = 255} 
#define SPHERES_COUNT 4
#define LIGHTS_COUNT 3
#define VIEW_PORT_DISTANCE 1
#define VIEWPORT_WIDTH 1
#define VIEWPORT_HEIGHT 1

int main(void) {
    const size_t width = 600;
    const size_t height = 600;
    colour_t pixels[width*height];
    canvas_t canvas = init_canvas(width, height, pixels);
    sphere_t spheres[SPHERES_COUNT] = {
        {(iv3_t){0, -1, 3}, 1, (colour_t){255, 0, 0}, 500},
        {(iv3_t){2, 0, 4}, 1, (colour_t){0, 255, 0}, 500},
        {(iv3_t){-2, 0, 4}, 1, (colour_t){0, 0, 255}, 10},
        {(iv3_t){0, -5001, 0}, 5000, (colour_t){255, 255, 0}, 1000}
    };
    light_t lights[LIGHTS_COUNT] = {
        {directional, 0.2, (iv3_t){1, 4, 4}},
        {point, 0.6, (iv3_t){2, 1, 0}},
        {ambient, 0.2},
    };

    iv3_t camera = {0, 0, 0};
    iv3_t transformed;

    for(int y = (int)-height / 2; y < (int)height / 2; y++) {
        for(int x = (int)-width / 2; x < (int)width / 2; x++) {
            transformed = canvas_to_viewport(&canvas, x, y);
            colour_t colour = trace_ray(camera, transformed, 1, INFINITY, spheres, lights);
            clamp_colour(&colour);
            put_pixel(&canvas, x, y, colour);
        }
    }

    render(&canvas, "ray-trace.ppm");
    return 0;
}

void render(canvas_t* canvas, const char* path) {
    FILE* out = fopen(path, "w");
    if (out == NULL)
        error_out("Could not open file %s", path);
    fprintf(out, "P3\n%zu %zu\n255\n", canvas->width, canvas->height);
    for(size_t i = 0; i < canvas->size; i++) {
        // printf("%i %i %i\n", canvas->pixels[i].r, canvas->pixels[i].g, canvas->pixels[i].b);
        fprintf(out, "%i %i %i\n", canvas->pixels[i].r, canvas->pixels[i].g, canvas->pixels[i].b);
    }
    fclose(out);
}

void put_pixel(canvas_t* can, int x, int y, colour_t col) {
    int screen_x = (int)(((double)can->width / 2) + x);
    int screen_y = (int)(((double)can->height / 2) - y - 1);
    can->pixels[can->width * screen_y + screen_x] = col;
}

colour_t trace_ray(iv3_t ray_origin, iv3_t transformed, double t_min, double t_max, sphere_t* spheres, light_t* lights) {
    sphere_t closest_sphere = {-1};
    double closest_intersection = INFINITY;
    for (size_t i = 0; i < SPHERES_COUNT; i++) {
        sphere_t sphere = spheres[i];
        iv2_t intersections = intersect_ray_sphere(ray_origin, transformed, sphere);
        if (intersections.x >= t_min && intersections.x <= t_max && intersections.x < closest_intersection) {
            closest_intersection = intersections.x;
            closest_sphere = spheres[i];
        }
        if (intersections.y >= t_min && intersections.y <= t_max && intersections.y < closest_intersection) {
            closest_intersection = intersections.y;
            closest_sphere = spheres[i];
        }
    }
    if (sphere_cmp(closest_sphere, (sphere_t){-1})/* sphere_cmp(closest_sphere, NULL_SPHERE) */) {
        return BACKGROUND_COLOUR;
    }

    iv3_t P = iv3_add(ray_origin, scalar_mul(transformed, closest_intersection));
    iv3_t N = iv3_sub(P, closest_sphere.center);
    N = scalar_div(N, iv3_length(N));
    double lighting = compute_lighting(P, N, scalar_mul(transformed, -1), closest_sphere.specular, lights);
    return scalar_mul_colour(closest_sphere.colour, lighting);
}

iv2_t intersect_ray_sphere(iv3_t origin, iv3_t transformed, sphere_t sphere) {
    iv3_t center = sphere.center;
    double radius = sphere.radius;
    iv2_t result;
    iv3_t oc;

    oc = iv3_sub(origin, center);
    double  a = iv3_dot(transformed, transformed);
    double  b = 2 * iv3_dot(oc, transformed);
    double  c = iv3_dot(oc, oc) - radius * radius;

    double discriminant = b*b - 4*a*c;
    if (discriminant < 0) {
        result.x = INFINITY;
        result.y = INFINITY;
        return result;
    }

    result.y = ((-b) + sqrt(discriminant)) / (2*a);
    result.x = ((-b) - sqrt(discriminant)) / (2*a);
    return result;
}


double compute_lighting(iv3_t position, iv3_t normal, iv3_t V, int specular, light_t* lights) {
    double I = 0.0;
    iv3_t L;
    for(int i = 0; i < LIGHTS_COUNT; i++) {
        if (lights[i].type == ambient) {
            I += lights[i].intensity;
        } else {
            if (lights[i].type == point) {
                L = iv3_sub(lights[i].position, position); 
            } else {
                L = lights[i].direction;
            }

            // diffuse
            double n_dot_1 = iv3_dot(normal, L);
            if (n_dot_1 > 0) {
                I += lights[i].intensity * n_dot_1/(iv3_length(normal) * iv3_length(L));
            }

            // specular
            if (specular != -1) {
                iv3_t R /* = iv3_sub(scalar_mul(scalar_mul(normal, 2), iv3_dot(normal, L)), L) */;
                R.x = 2 * normal.x * n_dot_1 - L.x;
                R.y = 2 * normal.y * n_dot_1 - L.y;
                R.z = 2 * normal.z * n_dot_1 - L.z;
                double r_dot_v = iv3_dot(R, V);
                if (r_dot_v > 0) {
                    I += lights[i].intensity * pow(r_dot_v/(iv3_length(R) * iv3_length(V)), specular);
                }
            }
        }
    }

    return I;
}

inline iv3_t canvas_to_viewport(canvas_t* can, int x, int y) {
    return (iv3_t){
        .x = (double)x * ((double)VIEWPORT_WIDTH / (double)can->width),
        .y = (double)y * ((double)VIEWPORT_HEIGHT / (double)can->height),
        .z = VIEW_PORT_DISTANCE,
    };
}

canvas_t init_canvas(size_t width, size_t height, colour_t pixels[width*height]) {
    canvas_t newc = (canvas_t){
        .width = width,
        .height = height,
        .size = width * height,
        .pixels = pixels,
    };

    memset(newc.pixels, 0, newc.size);
    return newc;
}

colour_t clamp_colour(colour_t* col) {
    if (col->r > 255)
        col->r = 255;
    else if (col->g > 255)
        col->g = 255;
    else if (col->b > 255)
        col->b = 255;
}

// vector maths
static inline iv3_t iv3_sub(iv3_t a, iv3_t b) {
    return (iv3_t) {
        .x = a.x - b.x,
        .y = a.y - b.y,
        .z = a.z - b.z,
    };
}

static inline double iv3_dot(iv3_t a, iv3_t b) {
    return (double){ 
        a.x*b.x + 
        a.y*b.y + 
        a.z*b.z 
    };
}

static inline iv3_t iv3_add(iv3_t a, iv3_t b) {
    return (iv3_t){
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
    };
}

static inline colour_t scalar_mul_colour(colour_t a, double b) {
    return (colour_t){
        a.r * b,
        a.g * b,
        a.b * b,
    };
}

static inline iv3_t scalar_mul(iv3_t a, double b) {
    return (iv3_t){
        a.x * b,
        a.y * b,
        a.z * b,
    };
}

static inline iv3_t scalar_div(iv3_t a, double b) {
    return (iv3_t){
        a.x / b,
        a.y / b,
        a.z / b,
    };
}

double iv3_length(iv3_t vec) {
    return (double)sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
}

// IMPORTANT: does not compair the colours
static inline double sphere_cmp(sphere_t a, sphere_t b) {
    if (iv3_cmp(a.center, b.center) &&
        a.radius == b.radius )
        return 1; // the same
    else
        return 0; // not the same
}

static inline double iv3_cmp(iv3_t a, iv3_t b) {
    if (a.x == b.x &&
        a.y == b.y &&
        a.z == b.z )
        return 1; // the same
    else
        return 0; // not the same
}

static inline double colour_cmp(colour_t a, colour_t b) {
    if (a.r == b.r &&
        a.g == b.g &&
        a.b == b.b )
        return 1; // the same
    else
        return 0; // not the same
}

void debug_iv3(iv3_t a) {
    printf("x:%f y:%f z:%f\n", a.x, a.y, a.z);
}

// taken from jacob sorber because im too dumb for this
void error_out(const char* fmt, ...) {
    va_list pa;
    int errno_save = errno;
    va_start(pa, fmt);
    vfprintf(stdout, fmt, pa);
    fprintf(stdout, "\n");
    fflush(stdout);
    if (errno_save != 0) {
        fprintf(stdout, "(errno = %d) : %s\n", errno_save, strerror(errno_save));
        fprintf(stdout, "\n");
        fflush(stdout);
    }
    va_end(pa);
}
