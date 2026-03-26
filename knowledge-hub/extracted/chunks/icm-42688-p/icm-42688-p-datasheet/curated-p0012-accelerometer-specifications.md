# Accelerometer specifications

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `12`
- Tags: `accel, sensitivity, startup, noise`

## Curated Summary

Accel supports ±16/8/4/2 g. Sensitivity at ±8 g is 4096 LSB/g. Typical accel startup time from sleep is 10 ms.

## Extracted Page Text

ICM-42688-P
Page 12 of 109
Document Number: DS-000347
Revision: 1.2
3.2 ACCELEROMETER SPECIFICATIONS
Typical Operating Circuit of section 4.2, VDD = 1.8 V, VDDIO = 1.8 V, TA=25°C, unless otherwise noted.
PARAMETER CONDITIONS MIN TYP MAX UNITS NOTES
ACCELEROMETER SENSITIVITY
Full-Scale Range
ACCEL_FS_SEL =0  ±16  g 2
ACCEL_FS_SEL =1  ±8  g 2
ACCEL_FS_SEL =2  ±4  g 2
ACCEL_FS_SEL =3  ±2  g 2
ADC Word Length Output in two’s complement format  16  bits 2, 5
Sensitivity Scale Factor
ACCEL_FS_SEL =0  2,048  LSB/g 2
ACCEL_FS_SEL =1  4,096  LSB/g 2
ACCEL_FS_SEL =2  8,192  LSB/g 2
ACCEL_FS_SEL =3  16,384  LSB/g 2
Sensitivity Scale Factor Initial Tolerance Component and Board-level, 25°C  ±0.5  % 1
Sensitivity Change vs. Temperature -40°C to +85°C  ±0.005  %/ºC 3
Nonlinearity Best Fit Straight Line, ±2g  ±0.1  % 3
Cross-Axis Sensitivity Board-level  ±1  % 3
ZERO-G OUTPUT
Initial Tolerance Board-level, all axes  ±20  mg 3
Zero-G Level Change vs. Temperature -40°C to +85°C  ±0.15  mg/ºC 3
OTHER PARAMETERS
Power Spectral Density @ 10 Hz X and Y-axis  65  µg/√Hz 1
Z-axis  70  µg/√Hz 1
RMS Noise Bandwidth = 100 Hz X and Y-axis  0.65  mg-rms 4
Z-axis  0.70  mg-rms 4
Low-Pass Filter Response ODR < 1kHz 5  500 Hz 2
ODR ≥ 1kHz 42  3979 Hz 2
Accelerometer Startup Time From sleep mode to valid data  10  ms 3
Output Data Rate  1.5625  32000 Hz 2
Table 2.  Accelerometer Specifications
Notes:
1. Tested in production.
2. Guaranteed by design.
3. Derived from validation or characterization of parts, not tested in production.
4. Calculated from Power Spectral Density.
5. 20-bits data format supported in FIFO, see section 6.1 .
