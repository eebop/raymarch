#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <iostream>
#include <thread>
#include <future>
#include <chrono>
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
    s->c.light = new af::array();
    *s->c.light = af::constant(-200, 1, 1, 3);
    return s;
}

void createSpheres(scene *s) {
    s->p.x = new af::array(1, 1, 3, 5);
    //*s->p.x = af::tile(af::randu(1, 1, 3, 5) * 100, 800, 800);
}

// t=0.892866, i=0.123837, j=-0.325027, k=0.286029

af::array rzerodist(af::array loc) {
    return af::rsqrt(af::sum(af::pow(loc, 2), 2));
}

af::array distg(af::array loc, scene *s) {
    // return af::sqrt(x * x + y * y + z * z) - 1;
    //return af::sqrt(af::min(af::sum(af::pow(af::tile(loc, 1, 1, 1, 5)-af::tile(*s->p.x, s->settings.windowx, s->settings.windowy), 2), 2), 3)) - 1;
    // auto a = loc - *s->p.x;
    // auto b = af::pow(a, 2);
    // auto c = af::sum(b, 2);
    // auto d = af::min(c, 3);
    // auto e = af::sqrt(d);
    // auto f = e - 1;
    // return f;
    ////return af::sqrt(af::min(af::sum(af::pow(loc - *s->p.x, 2), 2), 3)) - 1;
    af::array x = loc(af::span, af::span, 2);
    af::array y = af::sqrt(af::sum(af::pow(loc(af::span, af::span, af::seq(2)), 2), 2)) - 1.5;
    return af::sqrt(af::pow(x, 2) + af::pow(y, 2)) - 1;
    // return loc(af::span, af::span, 2);
}

// af::array dot(af::array x, af::array y, int axis) {
//     return af::sum(x + y, axis);
// }

// af::array cross(af::array x, af::array y) {
//     af::array out(x.dims(0), x.dims(1), 3);
//     out(af::span, af::span, 0) = x(af::span, af::span, 1) * y(af::span, af::span, 2) - x(af::span, af::span, 2) * y(af::span, af::span, 1);
//     out(af::span, af::span, 1) = x(af::span, af::span, 2) * y(af::span, af::span, 0) - x(af::span, af::span, 0) * y(af::span, af::span, 2);
//     out(af::span, af::span, 2) = x(af::span, af::span, 0) * y(af::span, af::span, 1) - x(af::span, af::span, 1) * y(af::span, af::span, 0);
//     return out;
// };

af::array deltadist(af::array loc, af::array vec, scene *s) {
    const double step = 0.0001;
    //af::print("t", distg(loc + vec * step, s) - distg(loc, s));
    return (- distg(vec * step + loc, s) + distg(loc, s)) / step;
}

af::array lighting(af::array delta, af::array ldelta, af::array lights, af::array halfdot, af::array ldist) {
    const float diffuse = 0.5;
    const float specular = 0.5;
    const float sharp = 27;
    // af::array l = af::acos(ldelta);
    // af::array v = af::acos( delta);
    // af::array a = af::acos(lights);
    return ((diffuse * af::max(0, ldelta) + af::pow(specular * af::max(0, halfdot), sharp)) * 0xB0).as(u32) * 0x010101;
    // return (af::pow(max(af::sin(a), 0), 2) * af::pow(a - (v + l), 2) * 0xB0).as(u32) * 0x010101;
}


// af::array normal(af::array points, af::array t1, af::array w, af::array t2, af::array v, scene *s) {
//     af::array x = cross(v, w);
//     af::array t3 = deltadist(x, points, s);
//     af::array mat = af::reorder(af::join(4, w, v, x), 2, 3, 0, 1);
//     af::array t = af::reorder(af::join(2, t1, t2, t3), 2, 3, 0, 1);
//     return af::solve(mat, t);
// }

void parellelStep(af::array vectors, double x, double y, double z, char *pixels, scene *s) {
    af::array points = af::join(2, af::constant(x, s->settings.windowx, s->settings.windowy), af::constant(y, s->settings.windowx, s->settings.windowy), af::constant(z, s->settings.windowx, s->settings.windowy));
    af::array distance(s->settings.windowx, s->settings.windowy);
    af::array mindist(s->settings.windowx, s->settings.windowy);
    af::setSeed(std::chrono::steady_clock::now().time_since_epoch().count());
    const int steps = 20;

    for (int c=0;c!=steps;c++) {
        distance = distg(points, s) + 0.1;
        points += distance * vectors;
    }


    af::array lightlenc = af::sqrt(af::sum(af::pow(points - *s->c.light, 2), 2));
    af::array lightlen = -lightlenc.copy();
    af::array lvectors = (points - *s->c.light) / lightlenc;
    af::array lpoints = af::tile(*s->c.light, s->settings.windowx, s->settings.windowy);
    for (int c=0;c!=steps;c++) {
        mindist = distg(lpoints, s) + 0.1;
        lpoints += mindist * lvectors;
        lightlen += mindist;
    }

    af::array delta = deltadist(points, vectors, s);
    af::array deltalight = deltadist(points, lvectors, s);
    af::array half = vectors + lvectors;
    half = half * rzerodist(half);
    af::array hdelta = deltadist(points, half, s);
    // //af::print("t", delta);
    af::array angle = af::sum(vectors * lvectors, 2) * rzerodist(vectors) * rzerodist(lvectors);
    // //af::array surfNormal = normal(points, delta, vectors, lvectors, deltalight, s);
    af::array lights = lighting(delta, deltalight, angle, hdelta, lightlenc);
    af::array dbool = (distance <= 0.001).as(u32);
    af::array lbool = (lightlen >= -0.001).as(u32);
    //af::print("f", lights(dbool != 0));
    //af::print("t", (af::randu(800, 800) > 0.5));
    //af::array colors = dbool * (0xB0B0B0 * lbool + 0x404040) + 0x0E0E0E + (0x010101 * (af::randu(800, 800) > 0.5).as(u32));
    af::array colors = dbool * (lights * lbool + 0x404040) + 0x0F0F0F;
    // af::print("c", colors);
    // af::print("t", colors(colors == 255));
    colors.host(pixels);
    // char *b = (distance <= 0.001).host<char>();

// #pragma omp parallel for
//     for (int q=0;q!=numpixels;q++) {
//         pixels[q * depth + 0] = r[q] * 255;std::chrono::milliseconds to unsigned long long
//         pixels[q * depth + 1] = r[q] * 255;
//         pixels[q * depth + 2] = r[q] * 255;

//         //pixels[q * depth + 1] = b[q] * 255;
//     }
//     af::freeHost((void *) r);
//     af::freeHost((void *) b);

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
    //vectors = vectors * rzerodist(vectors);
    parellelStep(vectors, s->c.cx, s->c.cy, s->c.cz, pixels, s);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    scene *s = allocScene();
    SDL_Window *w = SDL_CreateWindow("Test - Raymarch", 100, 100, s->settings.windowx, s->settings.windowy, SDL_TEXTUREACCESS_TARGET | SDL_WINDOW_RESIZABLE);
    SDL_Renderer *r = SDL_CreateRenderer(w, 0, SDL_RENDERER_SOFTWARE | SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    SDL_Event event;

    std::chrono::time_point<std::chrono::steady_clock> start;
    std::chrono::time_point<std::chrono::steady_clock> end;

    char *pixels;
    int pitch;
    createSpheres(s);
    while (1) {
        start = std::chrono::steady_clock::now();
        int x;
        int y;
        pitch = s->settings.windowx * 4;
        SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
        SDL_RenderClear(r);
        SDL_SetRenderDrawColor(r, 255, 0, 0, 255);

        SDL_Texture *buffer = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, s->settings.windowx, s->settings.windowy);
        SDL_LockTexture(buffer, NULL, (void **) &pixels, &pitch);
        std::thread gfxthread(setup, s, pixels);
        gfxthread.join();

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return 0;
            }
            update_debug(event, s);
            if (s->settings.grabMouse && event.type == SDL_MOUSEMOTION) {
                mouseUpdate(event, s);
            }

        }

        SDL_UnlockTexture(buffer);
        SDL_RenderCopy(r, buffer, NULL, NULL);
        SDL_RenderPresent(r);
        //printf("step done\n");
        end = std::chrono::steady_clock::now();
        std::chrono::milliseconds time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << time.count() << "ms\n";
    }
}