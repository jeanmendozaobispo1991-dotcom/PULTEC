# Modelo de graves (provisional)

Es la parte menos fundamentada. Las fuentes dan restricciones, no un esquema:

- Realce más empinado y atenuación que llega más arriba (Manley).
- Esquina de la atenuación ≈ ½ octava sobre la del realce (SOS 2019).
- Con 2,5/2,5: +3 dB de shelf y −2 dB de "dip" (SOS 2019).
- La posición del dip que da SOS (250 Hz–2 kHz según el selector) no es alcanzable con shelves
  simples sin romper lo demás; otras fuentes (MusicTech) lo sitúan unas ~7 veces por encima del selector.

## Qué hay implementado
- Realce: shelf resonante de 2.º orden en la frecuencia del selector (Q = 0,563).
- Atenuación: dos shelves de 1.er orden con esquinas en 0,673× y 11,76× el selector, repartiendo la
  profundidad (63,8 % / 36,2 %). Resultado a 2,5/2,5: +3,0 dB abajo, −2,0 dB de dip hacia 4× el selector.
- Curvas de potenciómetro: `dB = máx · (pos/10)^e` con e = 0,346 (realce) y 0,757 (atenuación),
  salidas del ajuste, no medidas.

Ajuste hecho con `scipy.optimize.least_squares` contra 3 rasgos × 4 selectores.

## Limitaciones
- No modela la interacción real (carga mutua) de la red pasiva; realce y atenuación son etapas en serie con
  formas asimétricas.
- Los exponentes y la posición del dip deben recalibrarse con curvas medidas del hardware o de un plugin de referencia.
- Cuando se tengan los valores de componentes, reemplazar por la red LC (`python/pultec/netlist.py`).
