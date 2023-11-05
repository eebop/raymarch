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


scene *allocScene(void) {
    scene *s = (scene *) malloc(sizeof(scene));
    s->c.cx = -8;
    s->c.cy = -8;
    s->c.cz = -8;
    s->c.q.i = 0.123837;
    s->c.q.j = -0.325027;
    s->c.q.k = 0.286029;
    s->c.q.t = 0.892866;
    s->settings.fov = 70;
    s->settings.grabMouse = 0;
    s->settings.windowx = 800;
    s->settings.windowy = 800;
    return s;
}

void createSpheres(scene *s) {
    s->p.x = new af::array(1, 1, 3, 5);
    *s->p.x = af::randu(1, 1, 3, 5) * 100;
}

// t=0.892866, i=0.123837, j=-0.325027, k=0.286029

af::array distg(af::array loc, scene *s) {
    // return af::sqrt(x * x + y * y + z * z) - 1;
    //return af::sqrt(af::min(af::sum(af::pow(af::tile(loc, 1, 1, 1, 5)-af::tile(*s->p.x, s->settings.windowx, s->settings.windowy), 2), 2), 3)) - 1;
    af::array x = loc(af::span, af::span, 2);
    af::array y = af::sqrt(af::sum(af::pow(loc(af::span, af::span, af::seq(2)), 2), 2)) - 1.5;
    return af::sqrt(af::pow(x, 2) + af::pow(y, 2)) - 1;
}

af::array distl(af::array loc, scene *s) {
    return af::sqrt(af::sum(af::pow(loc, 2), 2));
}

void parellelStep(af::array vectors, double x, double y, double z, char *pixels, scene *s) {
    af::array points = af::join(2, af::constant(x, s->settings.windowx, s->settings.windowy), af::constant(y, s->settings.windowx, s->settings.windowy), af::constant(z, s->settings.windowx, s->settings.windowy));
    af::array distance(s->settings.windowx, s->settings.windowy);
    af::array ldist(s->settings.windowx, s->settings.windowy);
    af::array mindist(s->settings.windowx, s->settings.windowy);

    for (int c=0;c!=15;c++) {
        distance = distg(points, s);
        points += af::tile(distance, 1, 1, 3) * vectors;

    }

    points = points + 2;
    vectors = -points * af::rsqrt(af::sum(af::pow(points, 2), 2));
    float add = 0.1;
    for (int c=0;c!=15;c++) {
        ldist = distl(points, s);
        mindist = af::min(distg(points, s) + add, ldist);
        points += af::tile(mindist, 1, 1, 3) * vectors;
        add = 0;
    }
    char *r = ((distance <= 0.001) && (ldist < 0.1)).host<char>();
    char *b = (distance <= 0.001).host<char>();
#pragma omp parallel for
    for (int q=0;q!=s->settings.windowx;q++) {
        for (int e=0;e!=s->settings.windowy;e++) {
            pixels[(q * s->settings.windowy + e) * 4 + 2] = r[q * s->settings.windowy + e] * 255;
            //pixels[(q * 800 + e) * 4 + 1] = r[q * 800 + e] * 255;
            pixels[(q * s->settings.windowy + e) * 4 + 1] = b[q * s->settings.windowy + e] * 255;
           // pixels[(q * 800 + e) * 4 + 3] = b[q * 800 + e] * 255;
        }
    }
    af::freeHost((void *) r);
    af::freeHost((void *) b);

}
void setup(scene *s, char *pixels) {
    af::array x = af::range(af::dim4(s->settings.windowx, s->settings.windowy));
    af::array y = af::range(af::dim4(s->settings.windowx, s->settings.windowy), 1);
    x = (x - s->settings.windowx / 2) / (180 * 800 / (s->settings.fov * M_PI));
    y = (y - s->settings.windowy / 2) / (180 * 800 / (s->settings.fov * M_PI));
    af::array z = af::rsqrt(x * x + y * y + 1);
    x = x * z;
    y = y * z;

    af::array _t =  - (z * s->c.q.i) - (y * s->c.q.j) - (x * s->c.q.k);
    af::array _i =    (z * s->c.q.t) - (y * s->c.q.k) + (x * s->c.q.j);
    af::array _j =    (z * s->c.q.k) + (y * s->c.q.t) - (x * s->c.q.i);
    af::array _k =  - (z * s->c.q.j) + (y * s->c.q.i) + (x * s->c.q.t);
    af::array vectors = af::join(2,
    (s->c.q.t * _i) - (s->c.q.i * _t) + (s->c.q.j * _k) - (s->c.q.k * _j),
    (s->c.q.t * _j) - (s->c.q.i * _k) - (s->c.q.j * _t) + (s->c.q.k * _i),
    (s->c.q.t * _k) + (s->c.q.i * _j) - (s->c.q.j * _i) - (s->c.q.k * _t));
    parellelStep(vectors, s->c.cx, s->c.cy, s->c.cz, pixels, s);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    af::setBackend(AF_BACKEND_OPENCL);
    scene *s = allocScene();
    SDL_Window *w = SDL_CreateWindow("Test - Raymarch", 100, 100, s->settings.windowx, s->settings.windowy, SDL_TEXTUREACCESS_TARGET | SDL_WINDOW_RESIZABLE);
    SDL_Renderer *r = SDL_CreateRenderer(w, 0, SDL_RENDERER_SOFTWARE | SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    quaternion pos;
    SDL_Event event;
    char *pixels;
    int pitch;
    createSpheres(s);
    while (1) {
        pitch = s->settings.windowx * 4;
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
        SDL_Texture *buffer = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, s->settings.windowx, s->settings.windowy);
        SDL_LockTexture(buffer, NULL, (void **) &pixels, &pitch);
        std::thread gfxthread(setup, s, pixels);

        gfxthread.join();
        SDL_UnlockTexture(buffer);
        SDL_RenderCopy(r, buffer, NULL, NULL);
        SDL_RenderPresent(r);
        //printf("step done\n");
    }
}