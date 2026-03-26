# Gyroscope specifications

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `11`
- Tags: `gyro, sensitivity, startup, noise`

## Curated Summary

Gyro supports ±2000 to ±15.625 dps. Sensitivity at ±2000 dps is 16.4 LSB/dps. Typical gyro startup time is 30 ms.

## Extracted Page Text

ICM-42688-P
Page 11 of 109
Document Number: DS-000347
Revision: 1.2
3 ELECTRICAL CHARACTERISTICS
3.1 GYROSCOPE SPECIFICATIONS
Typical Operating Circuit of section 4.2, VDD = 1.8 V, VDDIO = 1.8 V, TA=25°C, unless otherwise noted.
PARAMETER CONDITIONS MIN TYP MAX UNITS NOTES
GYROSCOPE SENSITIVITY
Full-Scale Range
GYRO_FS_SEL=0  ±2000  º/s 2
GYRO_FS_SEL =1  ±1000  º/s 2
GYRO_FS_SEL =2  ±500  º/s 2
GYRO_FS_SEL =3  ±250  º/s 2
GYRO_FS_SEL =4  ±125  º/s 2
GYRO_FS_SEL =5  ±62.5  º/s 2
GYRO_FS_SEL =6  ±31.25  º/s 2
GYRO_FS_SEL =7  ±15.625  º/s 2
Gyroscope ADC Word Length Output in two’s complement format  16  bits 2, 5
Sensitivity Scale Factor
GYRO_FS_SEL=0  16.4  LSB/(º/s) 2
GYRO_FS_SEL =1  32.8  LSB/(º/s) 2
GYRO_FS_SEL =2  65.5  LSB/(º/s) 2
GYRO_FS_SEL =3  131  LSB/(º/s) 2
GYRO_FS_SEL =4  262  LSB/(º/s) 2
GYRO_FS_SEL =5  524.3  LSB/(º/s) 2
GYRO_FS_SEL =6  1048.6  LSB/(º/s) 2
GYRO_FS_SEL =7  2097.2  LSB/(º/s) 2
Sensitivity Scale Factor Initial Tolerance Component and Board-level, 25°C  ±0.5  % 1
Sensitivity Scale Factor Variation Over
Temperature 0°C to +70°C  ±0.005  %/ºC 3
Nonlinearity Best fit straight line; 25°C  ±0.1  % 3
Cross-Axis Sensitivity Board-level  ±1.25  % 3
ZERO-RATE OUTPUT (ZRO)
Initial ZRO Tolerance Board-level, 25°C  ±0.5  º/s 3
ZRO Variation vs. Temperature 0°C to +70°C  ±0.005  º/s/ºC 3
OTHER PARAMETERS
Rate Noise Spectral Density @ 10 Hz  0.0028  º/s /√Hz 1
Total RMS Noise  Bandwidth = 100 Hz  0.028  º/s-rms 4
Gyroscope Mechanical Frequencies  25 27 29 KHz 1
Low Pass Filter Response ODR < 1kHz 5  500 Hz 2
ODR ≥ 1kHz 42  3979 Hz 2
Gyroscope Start-Up Time Time from gyro enable to gyro drive ready  30  ms 3
Output Data Rate  12.5  32000 Hz 2
Table 1. Gyroscope Specifications
Notes:
1. Tested in production.
2. Guaranteed by design.
3. Derived from validation or characterization of parts, not tested in production.
4. Calculated from Rate Noise Spectral Density.
5. 20-bits data format supported in FIFO, see section 6.1.
