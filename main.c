#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <math.h>
#include "main.h"
#include "events.h"

double dist(double i, double j, double k) {
    return sqrt(i * i + j * j + k * k) - 1;
}

data find_one(quaternion loc, quaternion *vec) {
    quaternion p = loc;
    //p.i = fmod(p.i, 10);
    double d = dist(p.i-5, p.j-0, p.k-0);
    data a;
    a.collision = 0;
    a.min_distance = d;
    a.steps = 0;
    while ((d > 0.1 && d < 1000) && a.steps < 500) {
        p.i += vec->i * d;
        p.j += vec->j * d;
        p.k += vec->k * d;
        //p.i = fmod(p.i, 10);
        if (d < a.min_distance) {
            a.min_distance = d;
        }
        a.steps++;
        d = dist(p.i-5, p.j, p.k);
    }
    if (a.min_distance <= 0.1) {
        a.collision = 255;
    }
    return a;
}

scene *allocScene(void) {
    scene *s = malloc(sizeof(scene));
    s->c.cx = 1;
    s->c.cy = 0;
    s->c.cz = 0;
    s->c.q.i = 0;
    s->c.q.j = 0;
    s->c.q.k = 0;
    s->c.q.t = 1;
    s->settings.fov = 70;
    s->settings.grabMouse = 0;
    return s;
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *w = SDL_CreateWindow("Test - Raymarch", 100, 100, 800, 800, SDL_TEXTUREACCESS_TARGET);
    SDL_Renderer *r = SDL_CreateRenderer(w, 0, SDL_RENDERER_SOFTWARE | SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    quaternion pos;
    quaternion dir;
    SDL_Event event;
    scene *s = allocScene();

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
        CREATE_QUATERNION(pos, s->c.cx, s->c.cy, s->c.cz);
        for (int x=0;x!=800;x++) {
            for (int y=0;y!=800;y++) {
                double xangle = (x - 400) / (180 * 800 / (s->settings.fov * M_PI));
                double yangle = (y - 400) / (180 * 800 / (s->settings.fov * M_PI));
                //printf("%f, %f\n", xangle, yangle);
                double val = sqrt(xangle * xangle + yangle * yangle + 1);
                CREATE_QUATERNION(dir, 1 / val, yangle / val, xangle / val);
                multiplyQuaternion(&dir, &(s->c.q), &dir);
                multiplyWithInverseFirstQuaternion(&(s->c.q), &dir, &dir);
                // if (x == 400 && y == 400) {
                //     DEBUG_QUATERNION(&dir);
                //     DEBUG_QUATERNION(&pos);
                // }
                data d = find_one(pos, &dir);
                SDL_SetRenderDrawColor(r, d.collision, d.min_distance, d.steps, 0xFF);
                SDL_RenderDrawPoint(r, x, y);
            }
        }
        SDL_RenderPresent(r);
    }
}