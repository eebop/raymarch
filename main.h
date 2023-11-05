#ifndef __MAIN_H
#define __MAIN_H
#include <math.h>
#include "quaternion.h"
#include <arrayfire.h>

#ifndef M_PI_2
#define M_PI_2 1.5707963267948966
#endif
#ifndef M_PI
#define M_PI 3.141592653589793
#endif 

typedef struct {
    int collision;
    int steps;
    double min_distance;
    double distance;
} data;

typedef struct {
    double cx;
    double cy;
    double cz;
    quaternion q;
} camera;

typedef struct {
    double fov;
    int grabMouse;
    int windowx;
    int windowy;
} scene_settings;

typedef struct {
    af::array *x;
    af::array *y;
    af::array *z;
} points;

typedef struct {
    points p;
    camera c;
    scene_settings settings;
} scene;

#endif