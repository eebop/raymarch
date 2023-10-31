#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <iostream>
#include <thread>
#include <future>
#include "main.h"
#include "events.h"

#include <arrayfire.h>

double dist(double i, double j, double k) {
    return sqrt(i * i + j * j + k * k) - 1;
}

data find_one(quaternion p, quaternion *vec) {
    //p.i = fmod(p.i, 10);
    double d;
    data a;
    a.collision = 0;
    a.min_distance = INFINITY;
    a.steps = 0;
    a.distance = 0;
    do {
        //p.j = fmod(p.j, 16);
        //p.k = fmod(p.k, 16);
        //p.i = fmod(p.i, 16);
        d = dist(p.i-8, p.j-8, p.k-8);
        p.i += vec->i * d;
        p.j += vec->j * d;
        p.k += vec->k * d;
        a.distance += d;
        if (d < a.min_distance) {
            a.min_distance = d;
        }
        a.steps++;
    } while ((d > 0.1 && d < 1000) && (a.steps < 1000));
    if (a.min_distance <= 0.1) {
        a.collision = 255;
    }
    return a;
}

scene *allocScene(void) {
    scene *s = (scene *) malloc(sizeof(scene));
    s->c.cx = 1;
    s->c.cy = 0;
    s->c.cz = 0;
    s->c.q.i = 0.123837;
    s->c.q.j = -0.325027;
    s->c.q.k = 0.286029;
    s->c.q.t = 0.892866;
    s->settings.fov = 70;
    s->settings.grabMouse = 0;
    return s;
}

// t=0.892866, i=0.123837, j=-0.325027, k=0.286029

af::array distg(af::array x, af::array y, af::array z) {
    return af::sqrt(x * x + y * y + z * z) - 1;
}

void parellelStep(af::array i, af::array j, af::array k, double x, double y, double z, char *pixels) {
    af::array xarr = af::constant(x, 800, 800);
    af::array yarr = af::constant(y, 800, 800);
    af::array zarr = af::constant(z, 800, 800);
    //af::array min_distance = af::constant(INFINITY, 800, 800);
    af::array distance(800, 800);
    //af::array collision = af::constant(0, 800, 800);
    for (int c=0;c!=50;c++) {
        distance = distg(xarr - 8, yarr - 8, zarr - 8);
        //min_distance(distance < min_distance) = distance(distance < min_distance);
        xarr += distance * i;
        yarr += distance * j;
        zarr += distance * k;
        xarr = af::mod(xarr, 16);
        yarr = af::mod(yarr, 16);
        zarr = af::mod(zarr, 16);

    }
    //printf("here\n");
    //min_distance = min_distance < 0.1;
    //distance = distance < 0.1;
    //distance.eval();
    //af::printMemInfo();
    char *min_host = (distance < 0.1).host<char>();
    for (int q=0;q!=800;q++) {
        for (int e=0;e!=800;e++) {
            //printf("checking, m is %f\n", min_host[q * 800 + e]);
            if (min_host[q * 800 + e]) {
                pixels[(q * 800 + e) * 4 + 0] = (char) 0xFF;
                pixels[(q * 800 + e) * 4 + 3] = (char) 0xFF;
            }
        }
    }
    //af::print("minimums: ", min_distance);
    //printf("min is %f\n", af::min<float>(min_distance));
    af::freeHost((void *) min_host);
}

void print_first(af::array x) {
    float *v = x.host<float>();
    printf("element (0, 0) is %f\n", *v);
    af::freeHost((void *)v);
}

void debugG(scene *s) {
    af::setBackend(AF_BACKEND_OPENCL);
    af::array x = af::range(af::dim4(800, 800));
    af::array y = af::range(af::dim4(800, 800), 1);
    x = (x - 400) / (180 * 800 / (s->settings.fov * M_PI));
    y = (y - 400) / (180 * 800 / (s->settings.fov * M_PI));
    af::array val = af::sqrt(x * x + y * y + 1);
    af::array z = 1 / val;
    //val.eval();
    x = x / val;
    y = y / val;
    print_first(x);
    print_first(y);
    print_first(z);
    af::array _t =  - (z * s->c.q.i) - (y * s->c.q.j) - (x * s->c.q.k);
    af::array _i =    (z * s->c.q.t) - (y * s->c.q.k) + (x * s->c.q.j);
    af::array _j =    (z * s->c.q.k) + (y * s->c.q.t) - (x * s->c.q.i);
    af::array _k =  - (z * s->c.q.j) + (y * s->c.q.i) + (x * s->c.q.t);
    af::array i = (s->c.q.t * _i) - (s->c.q.i * _t) + (s->c.q.j * _k) - (s->c.q.k * _j);
    af::array j = (s->c.q.t * _j) - (s->c.q.i * _k) - (s->c.q.j * _t) + (s->c.q.k * _i);
    af::array k = (s->c.q.t * _k) + (s->c.q.i * _j) - (s->c.q.j * _i) - (s->c.q.k * _t);
    print_first(i);
    print_first(j);
    print_first(k);
}

void debugC(scene *s) {
    double x = 0;
    double y = 0;
    double xangle = (x - 400) / (180 * 800 / (s->settings.fov * M_PI));
    double yangle = (y - 400) / (180 * 800 / (s->settings.fov * M_PI));
    //printf("%f, %f\n", xangle, yangle);
    double val = sqrt(xangle * xangle + yangle * yangle + 1);
    printf("%f, %f, %f\n", 1 / val, yangle / val, xangle / val);

    quaternion dir = quaternion(1 / val, yangle / val, xangle / val, 0);
    multiplyQuaternion(&dir, &(s->c.q), &dir);
    multiplyWithInverseFirstQuaternion(&(s->c.q), &dir, &dir);
    printf("%f, %f, %f\n", dir.i, dir.j, dir.k);
}

void setup(scene *s, char *pixels) {
    af::setBackend(AF_BACKEND_OPENCL);
    af::array x = af::range(af::dim4(800, 800));
    af::array y = af::range(af::dim4(800, 800), 1);
    x = (x - 400) / (180 * 800 / (s->settings.fov * M_PI));
    y = (y - 400) / (180 * 800 / (s->settings.fov * M_PI));
    af::array val = af::sqrt(x * x + y * y + 1);
    af::array z = 1 / val;
    //val.eval();
    x = x / val;
    y = y / val;

    af::array _t =  - (z * s->c.q.i) - (y * s->c.q.j) - (x * s->c.q.k);
    af::array _i =    (z * s->c.q.t) - (y * s->c.q.k) + (x * s->c.q.j);
    af::array _j =    (z * s->c.q.k) + (y * s->c.q.t) - (x * s->c.q.i);
    af::array _k =  - (z * s->c.q.j) + (y * s->c.q.i) + (x * s->c.q.t);
    af::array i = (s->c.q.t * _i) - (s->c.q.i * _t) + (s->c.q.j * _k) - (s->c.q.k * _j);
    af::array j = (s->c.q.t * _j) - (s->c.q.i * _k) - (s->c.q.j * _t) + (s->c.q.k * _i);
    af::array k = (s->c.q.t * _k) + (s->c.q.i * _j) - (s->c.q.j * _i) - (s->c.q.k * _t);
    parellelStep(i, j, k, s->c.cx, s->c.cy, s->c.cz, pixels);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *w = SDL_CreateWindow("Test - Raymarch", 100, 100, 800, 800, SDL_TEXTUREACCESS_TARGET);
    SDL_Renderer *r = SDL_CreateRenderer(w, 0, SDL_RENDERER_SOFTWARE | SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    quaternion pos;
    SDL_Event event;
    scene *s = allocScene();
    char *pixels;
    int pitch = 4 * 800;
    // debugC(s);
    // debugG(s);
    // return 0;
    // af::setBackend(AF_BACKEND_OPENCL);
    // af::array x = af::range(af::dim4(100, 100));
    // af::array y = af::range(af::dim4(100, 100), 1);
    // x = (x - 400) / (180 * 800 / (s->settings.fov * M_PI));
    // y = (y - 400) / (180 * 800 / (s->settings.fov * M_PI));
    // af::array val = x * x + y * y + 1;
    // af::array z = 1 / val;
    // x = x / val;
    // y = y / val;
    // af::array x = af::constant(0, 3)
    // af_array arr;
    // dim_t dims[2] = {1, 1};
    // af_randu(arr, 2, dims, f64);

    // std::future<int> val = std::async(&setup, s);
    // printf("out is %d\n", val.get());

    while (1) {
        SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
        SDL_RenderClear(r);
        SDL_SetRenderDrawColor(r, 255, 0, 0, 255);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return 0;
            }
            update_debug(event, s);
            if (s->settings.grabMouse && event.type == SDL_MOUSEMOTION) {
                mouseUpdate(event, s);
            }

        }
        pos = quaternion(s->c.cx, s->c.cy, s->c.cz, 0);
        //CREATE_QUATERNION(pos, s->c.cx, s->c.cy, s->c.cz);
        //DEBUG_QUATERNION(&(s->c.q))
        SDL_Texture *buffer = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 800, 800);
        SDL_LockTexture(buffer, NULL, (void **) &pixels, &pitch);
        std::thread gfxthread(setup, s, pixels);

// #pragma omp parallel for
//         for (int x=0;x!=800;x++) {
//             quaternion dir;
//             for (int y=0;y!=800;y++) {
//                 double xangle = (x - 400) / (180 * 800 / (s->settings.fov * M_PI));
//                 double yangle = (y - 400) / (180 * 800 / (s->settings.fov * M_PI));
//                 //printf("%f, %f\n", xangle, yangle);
//                 double val = sqrt(xangle * xangle + yangle * yangle + 1);
//                 dir = quaternion(1 / val, yangle / val, xangle / val, 0);
//                 multiplyQuaternion(&dir, &(s->c.q), &dir);
//                 multiplyWithInverseFirstQuaternion(&(s->c.q), &dir, &dir);
//                 // if (x == 400 && y == 400) {
//                 //     DEBUG_QUATERNION(&dir);
//                 //     DEBUG_QUATERNION(&pos);
//                 // }
//                 data d = find_one(pos, &dir);
//                 //SDL_SetRenderDrawColor(r, fmin(((double) d.collision * 100.0) / d.distance, 255), 0, 0, 0xFF);
//                 //if (d.collision) {
//                     //printf("yes, %d, %d\n", x, y);
//                 //}

//                 pixels[(y * 800 + x) * 4 + 0] = d.collision;
//                 pixels[(y * 800 + x) * 4 + 3] = d.collision;
//                 // if (d.collision)
//                 // SDL_RenderDrawPoint(r, x, y);
//             }
//         }
        gfxthread.join();
        SDL_UnlockTexture(buffer);
        SDL_RenderCopy(r, buffer, NULL, NULL);
        SDL_RenderPresent(r);
        //printf("step done\n");
    }
}