# Especificación: valores y fuentes

Estado: **[fuente]** publicado · **[derivado]** deducido de fuentes · **[supuesto]** decisión de diseño.

| Parámetro | Valor usado | Estado | Origen / nota |
|---|---|---|---|
| Frecuencias graves | 20/30/60/100 Hz | fuente | todas |
| Realce de graves máx. | 16 dB | fuente (discutido) | Manley, paper SMC 2024; MusicTech dice 13,5 |
| Atenuación de graves máx. | 20 dB | fuente (discutido) | Manley, paper; MusicTech 17,5; Waves 18 |
| Esquina de atenuación de graves | ~½ octava sobre el realce (+ shelf ancho) | derivado | SOS 2019 (Audio Precision) |
| Graves 2,5/2,5 | +3 dB (shelf), −2 dB (dip) | fuente | SOS 2019. Posición del dip: ajuste provisional (ver LF_MODEL.md) |
| Frecuencias realce agudos | 3/4/5/8/10/12/16 kHz | fuente | todas |
| Realce de agudos máx. | 20 dB (estrecho), 11 dB (ancho) | fuente | Manley/Waves 20 dB; SOS: 9 dB de diferencia |
| Tomas de bobina (L/C) | 29,9–87,9 mH / 3,3–18 nF | derivado | VintageWindings (valores revisados, no los del Pultec original); f0 = 1/(2π√LC) coincide con 4–16 kHz |
| Q según frecuencia | ∝ √(L/C)/R | derivado | regresión sobre las tomas |
| Rango de ancho de banda (Q) | 2,5 (estrecho) – 0,6 (ancho) | supuesto | a calibrar |
| Atenuación de agudos | 5/10/20 kHz, −21 dB máx., −12 dB en la f. elegida | fuente | Manley (paper 20 dB; MusicTech 16 dB) |
| Pérdida de inserción | ~16 dB (recuperada por el amplificador) | fuente | paper SMC 2024, AudioTechnology, GearNews (otras citan ~20 dB) |
| Impedancia de la red | 600 Ω | fuente | SOS 2019 |
| Válvulas | 12AX7 (entrada), 12AU7 (salida), push-pull con realimentación | fuente | SOS 2019 / listados |
| THD original | ≤ 0,15 % a +10 dBm | fuente | ficha original |
| Armónicos | H2 crece con el nivel; resto ~constante | fuente | SOS 2019 (medición real) |
| Saturación de la sección de graves | transformador, solo < ~120 Hz | supuesto | modelo conductual |
| Nivel de referencia | −18 dBFS = +4 dBu | supuesto | |
| Fase | mínima | fuente | circuito pasivo |
| Oversampling | 2/4/8x, prewarp | derivado | paper SMC 2024: el realce de agudos es la sección más sensible |

## Ponderación para calibrar (Fletcher-Munson / ISO 226, solo como guía)
Rejilla densa en 40–150 Hz y 2–12 kHz; tolerancia orientativa 0,5–1 dB ahí y 1,5–2 dB en
extremos (<30 Hz, >14 kHz). Probar a 2–3 niveles de entrada.

## Por qué los plugins suenan distinto (SOS 2020, H. Weekhout)
La mayoría no tiene control de entrada: el nivel decide la coloración; cada desarrollador modela una
"unidad dorada" distinta; unos suben la distorsión de forma proporcional, otros con umbral y otros
no distorsionan. Por eso este proyecto incluye **Input Gain**, **Auto Gain** y una curva de saturación
progresiva sin umbral explosivo.

## Fuentes consultadas
- Sound on Sound, *Pulse Techniques EQP-1A* (2019, mediciones Audio Precision) y *The Hidden Dangers Of Vintage EQ* (2020)
- Manley Enhanced Pultec EQP-1A, manual del propietario
- Barrera et al., *Modeling the Pultec EQP-1A with Wave Digital Filters*, SMC 2024
- Manuales de Waves PuigTec y UAD; AudioTechnology; GearNews; MusicTech; VintageWindings
