# Programmable interrupts overview

- Chip: `ICM-42688-P`
- Chip Slug: `icm-42688-p`
- Document: `icm-42688-p-datasheet`
- Document Kind: `datasheet`
- Source PDF: `hardware-docs/ICM-42688-P_datasheet.pdf`
- Page: `41`
- Tags: `interrupts, int1, int2`

## Curated Summary

INT1 and INT2 can be configured as push-pull or open-drain, level or pulse, active-high or active-low.

## Extracted Page Text

ICM-42688-P
Page 41 of 109
Document Number: DS-000347
Revision: 1.2
7 PROGRAMMABLE INTERRUPTS
The ICM-42688-P has a programmable interrupt system that can generate an interrupt signal on the INT pins. Status flags indicate
the source of an interrupt. Interrupt sources may be enabled and disabled  individually. There are two interrupt outputs. Any
interrupt may be mapped to either interrupt pin as explained in the register section.  The following configuration options are
available for the interrupts
• INT1 and INT2 can be push-pull or open drain
• Level or pulse mode
• Active high or active low
Additionally, ICM-42688-P includes In-band Interrupt (IBI) support for the I3CSM interface.
