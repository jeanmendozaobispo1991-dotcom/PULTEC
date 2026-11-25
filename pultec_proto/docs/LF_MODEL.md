# Modelo de graves (provisional, v0.3: realce máx. 18 dB)

Es la parte menos fundamentada. Las fuentes dan restricciones, no un esquema:

- Realce más empinado y atenuación que llega más arriba (Manley).
- La frecuencia elegida queda **cerca del máximo** del realce, con algo más de ganancia por debajo (Manley).
- Con 2,5/2,5: +3 dB de shelf y -2 dB de "dip" (SOS 2019).
- Realce y atenuación a la vez dan más grave con un hueco por encima ("truco Pultec").

## Qué hay implementado
- **Realce:** shelf resonante de 2.º orden (Q = 0,6) cuyo centro se calcula para que la ganancia *en la
  frecuencia elegida* sea el máximo menos 3 dB. (v0.1 ponía ahí la mitad de la ganancia, y por eso a 60 Hz
  con el realce al máximo solo subía +8 dB; ahora sube +13 dB, y todavía +9 dB a 100 Hz.)
- **Atenuación:** dos shelves de 1.er orden (esquinas 0,4675x y 32x el selector, 53,65 % / 46,35 % de la
  profundidad).
- **Interacción:** el realce reduce la profundidad efectiva de la atenuación: `c_ef = c * (1 - 0,6 * b/16)`.
  Sin esto, con ambos al máximo el grave neto salía negativo (16 - 20 dB), lo contrario del truco real.
- **Potenciómetros:** `dB = máx * (pos/10)^e`, e = 0,5 (realce) y 0,7 (atenuación). Supuestos, no medidos.

## Resultados del modelo
| Ajuste | Resultado |
|---|---|
| Realce 10 solo, 60 Hz | +16,5 dB a 60 Hz, +12 dB a 100 Hz |
| Atenuación 10 sola | -14 a -19 dB en graves (fuentes: 17,5-20) |
| 2,5 / 2,5 | +3,0 dB abajo, dip -2,25 dB a ~7x el selector |
| 10 / 10 | ~+11 dB en el selector, dip ~-3 dB por encima |

## Limitaciones
- No es la red LC real: realce y atenuación son etapas con una interacción simplificada.
- Exponentes, Q, interacción y posición del dip deben recalibrarse con curvas medidas del hardware o de
  un plugin de referencia (ver README, hoja de ruta).
