# Temperature and power modes

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `24`
- Tags: `temperature, power, formula`

## Curated Summary

Temperature conversion for sensor registers is raw/132.48 + 25. User-visible modes include sleep, standby, accel LP/LN, gyro LN, and 6-axis LN.

## Extracted Page Text

ICM-42688-P
Page 24 of 109
Document Number: DS-000347
Revision: 1.2
4.13 DIGITAL-OUTPUT TEMPERATURE SENSOR
An on-chip temperature sensor and ADC are used to measure the ICM-42688-P die temperature.  The readings from the ADC can be
read from the FIFO or the Sensor Data registers.
Temperature sensor register data TEMP_DATA is updated with new data at max(Accelerometer ODR, Gyroscope ODR).
Temperature data value from the sensor data registers can be converted to degrees centigrade by using the following formula:
Temperature in Degrees Centigrade = (TEMP_DATA / 132.48) + 25
Temperature data stored in FIFO is an 8-bit quantity, FIFO_TEMP_DATA.  It can be converted to degrees centigrade by using the
following formula:
Temperature in Degrees Centigrade = (FIFO_TEMP_DATA / 2.07) + 25
4.14 BIAS AND LDOS
The bias and LDO section generates the internal supply and the reference voltages and currents required by the ICM-42688-P.
4.15 CHARGE PUMP
An on-chip charge pump generates the high voltage required for the MEMS oscillator.
4.16 STANDARD POWER MODES
The following table lists the user-accessible power modes for ICM-42688-P.
Mode Name Gyro Accel
1 Sleep Mode Off Off
2 Standby Mode Drive On Off
3 Accelerometer Low-Power Mode Off Duty-Cycled
4 Accelerometer Low-Noise Mode Off On
5 Gyroscope Low-Noise Mode On Off
6 6-Axis Low-Noise Mode On On
Table 11. Standard Power Modes for ICM-42688-P
