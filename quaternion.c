#include <stdlib.h>
#include <immintrin.h>
#include "quaternion.h"

quaternion *newQuaternion(void) {
    quaternion *q = malloc(sizeof(quaternion));
    return q;
}

#ifdef __AVX__
void multiplyQuaternion(quaternion *q1, quaternion *q2, quaternion *qout) {
    __m256d t = _mm256_loadu_pd((double *) q2);
    __m256d i = _mm256_permute_pd(t, 5);
    __m256d j = _mm256_castsi256_pd(_mm256_permute2x128_si256(_mm256_castpd_si256(t), _mm256_castpd_si256(t), 1));
    __m256d k = _mm256_permute_pd(j, 5);
    t = _mm256_mul_pd(t, _mm256_set_pd( q1->t,  q1->t,  q1->t,  q1->t));
    i = _mm256_mul_pd(i, _mm256_set_pd(-q1->i,  q1->i,  q1->i, -q1->i));
    j = _mm256_mul_pd(j, _mm256_set_pd( q1->j,  q1->j, -q1->j, -q1->j));
    k = _mm256_mul_pd(k, _mm256_set_pd( q1->k, -q1->k,  q1->k, -q1->k));
    __m256d qout_ = _mm256_add_pd(_mm256_add_pd(t, i), _mm256_add_pd(j, k));
    _mm256_storeu_pd((double *) qout, qout_);
}
void multiplyWithInverseFirstQuaternion(quaternion *q1, quaternion *q2, quaternion *qout) {
    __m256d t = _mm256_loadu_pd((double *) q2);
    __m256d i = _mm256_permute_pd(t, 5);
    __m256d j = _mm256_castsi256_pd(_mm256_permute2x128_si256(_mm256_castpd_si256(t), _mm256_castpd_si256(t), 1));
    __m256d k = _mm256_permute_pd(j, 5);
    t = _mm256_mul_pd(t, _mm256_set_pd( q1->t,  q1->t,  q1->t,  q1->t));
    i = _mm256_mul_pd(i, _mm256_set_pd( q1->i, -q1->i, -q1->i,  q1->i));
    j = _mm256_mul_pd(j, _mm256_set_pd(-q1->j, -q1->j,  q1->j,  q1->j));
    k = _mm256_mul_pd(k, _mm256_set_pd(-q1->k,  q1->k, -q1->k,  q1->k));
    __m256d qout_ = _mm256_add_pd(_mm256_add_pd(t, i), _mm256_add_pd(j, k));
    _mm256_storeu_pd((double *) qout, qout_);
}
void multiplyWithInverseSecondQuaternion(quaternion *q1, quaternion *q2, quaternion *qout) {
    __m256d t = _mm256_loadu_pd((double *) q2);
    __m256d i = _mm256_permute_pd(t, 5);
    __m256d j = _mm256_castsi256_pd(_mm256_permute2x128_si256(_mm256_castpd_si256(t), _mm256_castpd_si256(t), 1));
    __m256d k = _mm256_permute_pd(j, 5);
    t = _mm256_mul_pd(t, _mm256_set_pd(-q1->t, -q1->t, -q1->t,  q1->t));
    i = _mm256_mul_pd(i, _mm256_set_pd( q1->i, -q1->i,  q1->i,  q1->i));
    j = _mm256_mul_pd(j, _mm256_set_pd(-q1->j,  q1->j,  q1->j,  q1->j));
    k = _mm256_mul_pd(k, _mm256_set_pd( q1->k,  q1->k, -q1->k,  q1->k));
    __m256d qout_ = _mm256_add_pd(_mm256_add_pd(t, i), _mm256_add_pd(j, k));
    _mm256_storeu_pd((double *) qout, qout_);
}

#else
// Temp variable is so that operations like multiply(a, b, a) are ok
void multiplyQuaternion(quaternion *q1, quaternion *q2, quaternion *qout) {
    quaternion qtemp;
    qtemp.t = (q1->t * q2->t) - (q1->i * q2->i) - (q1->j * q2->j) - (q1->k * q2->k);
    qtemp.i = (q1->t * q2->i) + (q1->i * q2->t) - (q1->j * q2->k) + (q1->k * q2->j);
    qtemp.j = (q1->t * q2->j) + (q1->i * q2->k) + (q1->j * q2->t) - (q1->k * q2->i);
    qtemp.k = (q1->t * q2->k) - (q1->i * q2->j) + (q1->j * q2->i) + (q1->k * q2->t);
    qout->t = qtemp.t;
    qout->i = qtemp.i;
    qout->j = qtemp.j;
    qout->k = qtemp.k;
}
void multiplyWithInverseFirstQuaternion(quaternion *q1, quaternion *q2, quaternion *qout) {
    quaternion qtemp;
    qtemp.t = (q1->t * q2->t) + (q1->i * q2->i) + (q1->j * q2->j) + (q1->k * q2->k);
    qtemp.i = (q1->t * q2->i) - (q1->i * q2->t) + (q1->j * q2->k) - (q1->k * q2->j);
    qtemp.j = (q1->t * q2->j) - (q1->i * q2->k) - (q1->j * q2->t) + (q1->k * q2->i);
    qtemp.k = (q1->t * q2->k) + (q1->i * q2->j) - (q1->j * q2->i) - (q1->k * q2->t);
    qout->t = qtemp.t;
    qout->i = qtemp.i;
    qout->j = qtemp.j;
    qout->k = qtemp.k;
}
void multiplyWithInverseSecondQuaternion(quaternion *q1, quaternion *q2, quaternion *qout) {
    quaternion qtemp;
    qtemp.t = (q1->t * q2->t) + (q1->i * q2->i) + (q1->j * q2->j) + (q1->k * q2->k);
    qtemp.i =-(q1->t * q2->i) + (q1->i * q2->t) + (q1->j * q2->k) - (q1->k * q2->j);
    qtemp.j =-(q1->t * q2->j) - (q1->i * q2->k) + (q1->j * q2->t) + (q1->k * q2->i);
    qtemp.k =-(q1->t * q2->k) + (q1->i * q2->j) - (q1->j * q2->i) + (q1->k * q2->t);
    qout->t = qtemp.t;
    qout->i = qtemp.i;
    qout->j = qtemp.j;
    qout->k = qtemp.k;

}
#endif