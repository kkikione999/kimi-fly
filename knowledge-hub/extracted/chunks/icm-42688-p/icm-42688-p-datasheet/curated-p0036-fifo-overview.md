# FIFO overview

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `36`
- Tags: `fifo, buffer, 20bit`

## Curated Summary

FIFO size is 2 KB. FIFO can hold accel, gyro, temperature, and timestamp data. 20-bit FIFO mode has FSR constraints and should not be the initial bring-up path.

## Extracted Page Text

ICM-42688-P
Page 36 of 109
Document Number: DS-000347
Revision: 1.2
6 FIFO
The ICM-42688-P contains a 2K byte FIFO register that is accessible via the serial interface.  The FIFO configuration register
determines which data is written into the FIFO.  Possible choices include gyroscope data, accelerometer data, temperature readings,
and FSYNC input.  A FIFO counter keeps track of how many bytes of valid data are contained in the FIFO.
6.1 PACKET STRUCTURE
The following figure shows the FIFO packet structures supported in ICM-42688-P.  Base data format for gyroscope and
accelerometer is 16-bits per element.  20-bits data format support is included in one of the packet structures.  When 20 -bits data
format is used, gyroscope data consists of 19-bits of actual data and the LSB is always set to 0, accelerometer data consists of 18-bits
of actual data and the two lowest order bits are always set to 0.   When 20-bits data format is used, the only FSR settings that are
operational are ±2000dps for gyroscope and ±16g for accelerometer, even if the FSR selection register settings are configured for
other FSR values.  The corresponding sensitivity scale factor values are 131 LSB/dps for gyroscope and 8192 LSB/g for
accelerometer.
Header
(1 byte)
Accelerometer Data
(6 bytes)
Temperature Data
(1 byte)
Header
(1 byte)
Gyroscope Data
(6 bytes)
Temperature Data
(1 byte)
Header
(1 byte)
Accelerometer Data
(6 bytes)
Gyroscope Data
(6 bytes)
TimeStamp
(2 bytes)
Temperature Data
(1 byte)
Header
(1 byte)
Accelerometer Data
(6 bytes)
Gyroscope Data
(6 bytes)
TimeStamp
(2 bytes)
Temperature Data
(2 bytes)
20-bit Extension
(3 bytes)
Packet 1 Packet 2
Packet 3
Packet 4
Figure 9. FIFO Packet Structure
The rest of this sub-section describes how individual data is packaged in the different FIFO packet structures.
