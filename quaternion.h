#ifndef __QUATERNION_H
#define __QUATERNION_H

#define CREATE_QUATERNION(p, _i, _j, _k) p .i = _i ; p .j = _j ; p .k = _k ; p .t = 0;
#define DEBUG_QUATERNION(p) printf("t=%f, i=%f, j=%f, k=%f\n", ( p ) ->t, ( p ) ->i, ( p ) ->j, ( p ) ->k);

typedef struct {
    double t;
    double i;
    double j;
    double k;
} quaternion;
quaternion *newQuaternion(void);
void multiplyQuaternion(quaternion *q1, quaternion *q2, quaternion *qout);
void multiplyWithInverseSecondQuaternion(quaternion *q1, quaternion *q2_, quaternion *qout);
void multiplyWithInverseFirstQuaternion(quaternion *q1, quaternion *q2, quaternion *qout);
#endif