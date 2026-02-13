/*
 * DSP_Biquad.h
 * Descripción: Filtro IIR de 2do Orden (Biquad) portable.
 * Soporta: LowPass, HighPass, BandPass, Notch.
 */

#ifndef DSP_BIQUAD_H_
#define DSP_BIQUAD_H_

#include <stdint.h>
#include <math.h>

/* Tipos de Filtro Disponibles */
typedef enum {
    BQ_LOWPASS,
    BQ_HIGHPASS,
    BQ_BANDPASS,
    BQ_NOTCH
} BiquadFilterType;

/* Estructura de coeficientes y estado */
typedef struct {
    // Coeficientes calculados
    float b0, b1, b2;
    float a1, a2;

    // Variables de estado (historia)
    float x1, x2; // x[n-1], x[n-2]
    float y1, y2; // y[n-1], y[n-2]
} Biquad_t;

/**
 * @brief Inicializa y calcula los coeficientes del filtro.
 * @param f: Puntero a la estructura del filtro.
 * @param type: Tipo de filtro (BQ_LOWPASS, BQ_HIGHPASS, etc.)
 * @param fc: Frecuencia de corte (Hz).
 * @param fs: Frecuencia de muestreo (Hz) -> (1/ts).
 * @param Q: Factor de calidad (típicamente 0.707 para respuesta plana Butterworth).
 */
void Biquad_Init(Biquad_t *f, BiquadFilterType type, float fc, float fs, float Q);

/**
 * @brief Resetea la historia del filtro (limpia el buffer).
 */
void Biquad_Reset(Biquad_t *f);

/**
 * @brief Aplica el filtro a una nueva muestra.
 * @param x: Muestra de entrada actual.
 * @return float: Muestra filtrada.
 */
float Biquad_Apply(Biquad_t *f, float x);

#endif /* DSP_BIQUAD_H_ */
