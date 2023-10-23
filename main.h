#ifndef __MAIN_H
#define __MAIN_H
#include <math.h>
#include "quaternion.h"

#ifndef M_PI_2
#define M_PI_2 1.5707963267948966
#endif
#ifndef M_PI
#define M_PI 3.141592653589793
#endif 

typedef struct {
    int collision;
    int steps;
    int min_distance;
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
} scene_settings;

typedef struct {
    camera c;
    scene_settings settings;
} scene;

#endif