#include <SDL2/SDL.h>
#include <stdio.h>
#include "main.h"
#include "quaternion.h"

static quaternion rotations[6] = {
    quaternion(-0.024541228522912288, 0, 0, 0.9996988186962042),
    quaternion(0, -0.024541228522912288, 0, 0.9996988186962042),
    quaternion(0, 0, -0.024541228522912288, 0.9996988186962042),
    quaternion(0.024541228522912288, 0, 0,  0.9996988186962042),
    quaternion(0, 0.024541228522912288, 0,  0.9996988186962042),
    quaternion(0, 0, 0.024541228522912288,  0.9996988186962042)
};
/*
    {0.9996988186962042, -0.024541228522912288, 0, 0},
    {0.9996988186962042, 0, -0.024541228522912288, 0},
    {0.9996988186962042, 0, 0, -0.024541228522912288},
    {0.9996988186962042,  0.024541228522912288, 0, 0},
    {0.9996988186962042,  0, 0.024541228522912288, 0},
    {0.9996988186962042,  0, 0, 0.024541228522912288}
};
*/

void submitQuaternionRotation(scene *s, quaternion *rotation) {
    multiplyQuaternion(rotation, &(s->c.q), &(s->c.q));
    //multiplyQuaternion(s->c->q, rotation, s->c->q);
}

void submitRotation(scene *s, int rotation, int direction) {
    submitQuaternionRotation(s, &(rotations[rotation + (3 * direction)]));
}

void mouseUpdate(SDL_Event event, scene *s) {
    // we can't use xrel and yrel because warping generates a mousemotion event
    //printf("%d %d\n", event.motion.x, event.motion.y);
    quaternion x = quaternion(0, SDL_sin((-(double)event.motion.xrel) / 1024.0), 0, SDL_cos((-(double)event.motion.xrel) / 1024.0));
    quaternion y = quaternion(0, 0, SDL_sin(( (double)event.motion.yrel) / 1024.0), SDL_cos(( (double)event.motion.yrel) / 1024.0));
    submitQuaternionRotation(s, &x);
    submitQuaternionRotation(s, &y);
}

void update_debug(SDL_Event event, scene *s) {
    quaternion q;
    float *lightdata;
    double i = 0;
    double j = 0;
    double k = 0;
    double shift_multiplier = 1;
    if (SDL_GetModState() & KMOD_SHIFT) {
        shift_multiplier = 0.1;
    }
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_w:
            i++;
            break;
        case SDLK_s:
            i--;
            break;
        case SDLK_q:
            j++;
            break;
        case SDLK_e:
            j--;
            break;
        case SDLK_d:
            k++;
            break;
        case SDLK_a:
            k--;
            break;
        case SDLK_LEFT:
            submitRotation(s, 1, 1);
            break;
        case SDLK_RIGHT:
            submitRotation(s, 1, 0);
            break;
        case SDLK_DOWN:
            submitRotation(s, 2, 1);
            break;
        case SDLK_UP:
            submitRotation(s, 2, 0);
            break;
        case SDLK_k:
            *s->c.light = af::randu(1, 1, 3) * 10 - 5;
            break;
        case SDLK_j:
            lightdata = s->c.light->host<float>();
            s->c.cx = lightdata[0];
            s->c.cy = lightdata[1];
            s->c.cz = lightdata[2];
            af::freeHost((void *) lightdata);
            break;
        case SDLK_l:
            if (s->settings.grabMouse) {
                SDL_SetRelativeMouseMode(SDL_FALSE);
                s->settings.grabMouse = 0;

            } else {
                s->settings.grabMouse = 1;
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }
            break;
        default:
            printf("unknown key: %d\n", event.key.keysym.sym);
            break;
        }
    }
    if (event.type == SDL_MOUSEBUTTONUP) {
        switch (event.button.button)
        {
        case SDL_BUTTON_X1:
            submitRotation(s, 0, 0);
            break;
        case SDL_BUTTON_X2:
            submitRotation(s, 0, 1);
            break;
        default:
            break;
        }
    }
    if (event.type == SDL_WINDOWEVENT) {
        switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
                s->settings.windowx = event.window.data1;
                s->settings.windowy = event.window.data2;
                printf("size is %d, %d\n", s->settings.windowx, s->settings.windowy);
            break;
            default:
            break;
        }
    }
    q = quaternion(i, j, k, 0);
    multiplyWithInverseFirstQuaternion(&(s->c.q), &q, &q);
    multiplyQuaternion(&q, &(s->c.q), &q);

    s->c.cx += q.i * shift_multiplier;
    s->c.cy += q.j * shift_multiplier;
    s->c.cz += q.k * shift_multiplier;

}