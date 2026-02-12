/*
 * DSP_Biquad.c
 */

#include "DSP_Biquad.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void Biquad_Init(Biquad_t *f, BiquadFilterType type, float fc, float fs, float Q) {
    // Limpiamos memoria de estado
    Biquad_Reset(f);

    float w0 = 2.0f * M_PI * fc / fs;
    float alpha = sinf(w0) / (2.0f * Q);
    float cos_w0 = cosf(w0);

    // Variables temporales para coeficientes sin normalizar
    float a0_inv = 1.0f; // Inverso de a0 para normalizar
    float b0_t, b1_t, b2_t, a0_t, a1_t, a2_t;

    switch (type) {
        case BQ_LOWPASS:
            b0_t = (1.0f - cos_w0) / 2.0f;
            b1_t = 1.0f - cos_w0;
            b2_t = (1.0f - cos_w0) / 2.0f;
            a0_t = 1.0f + alpha;
            a1_t = -2.0f * cos_w0;
            a2_t = 1.0f - alpha;
            break;

        case BQ_HIGHPASS:
            b0_t = (1.0f + cos_w0) / 2.0f;
            b1_t = -(1.0f + cos_w0);
            b2_t = (1.0f + cos_w0) / 2.0f;
            a0_t = 1.0f + alpha;
            a1_t = -2.0f * cos_w0;
            a2_t = 1.0f - alpha;
            break;

        case BQ_BANDPASS:
            b0_t = alpha;
            b1_t = 0.0f;
            b2_t = -alpha;
            a0_t = 1.0f + alpha;
            a1_t = -2.0f * cos_w0;
            a2_t = 1.0f - alpha;
            break;

        case BQ_NOTCH:
            b0_t = 1.0f;
            b1_t = -2.0f * cos_w0;
            b2_t = 1.0f;
            a0_t = 1.0f + alpha;
            a1_t = -2.0f * cos_w0;
            a2_t = 1.0f - alpha;
            break;

        default:
            // Por defecto Pass-Through (sin filtro)
            f->b0 = 1.0f; f->b1 = 0.0f; f->b2 = 0.0f;
            f->a1 = 0.0f; f->a2 = 0.0f;
            return;
    }

    // Normalización: Dividimos todo por a0 (para que a0 sea 1 en la formula final)
    a0_inv = 1.0f / a0_t;

    f->b0 = b0_t * a0_inv;
    f->b1 = b1_t * a0_inv;
    f->b2 = b2_t * a0_inv;
    f->a1 = a1_t * a0_inv;
    f->a2 = a2_t * a0_inv;
}

void Biquad_Reset(Biquad_t *f) {
    f->x1 = 0.0f;
    f->x2 = 0.0f;
    f->y1 = 0.0f;
    f->y2 = 0.0f;
}

float Biquad_Apply(Biquad_t *f, float x) {
    // Ecuación de Diferencias: Direct Form I
    // y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]

    float y = (f->b0 * x) + (f->b1 * f->x1) + (f->b2 * f->x2)
              - (f->a1 * f->y1) - (f->a2 * f->y2);

    // Desplazar historia (shift buffer)
    f->x2 = f->x1;
    f->x1 = x;

    f->y2 = f->y1;
    f->y1 = y;

    return y;
}
