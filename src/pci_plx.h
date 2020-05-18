
#ifndef __pci_plx__
#define __pci_plx__

#define PCI_ID_PLX 0x903010b5
#define PCI_ID_DM642 0x9065104c
#define PCI_ID_CIFX50_HBOOT 0x000015cf
#define PCI_ID_PLX_BRIDGE 0x811210b5
#define PCI_ID_XIO_BRIDGE 0x8240104C
#define PCI_ID_CIFX4000 0x400015cf
#define PCI_ID_CIFXM2 0x009015cf

#define PCI_ADR_DevId_VendId                    0x00
#define PCI_ADR_Status_Cmd                      0x04
#define PCI_ADR_ClassCode_RevId                 0x08
#define PCI_ADR_HdrType_LatTimer_CacheSize      0x0c
#define PCI_ADR_MemBar0                         0x10
#define PCI_ADR_MemBar1                         0x14
#define PCI_ADR_MemBar2                         0x18
#define PCI_ADR_MemBar3                         0x1c
#define PCI_ADR_MemBar4                         0x20
#define PCI_ADR_MemBar5                         0x24
#define PCI_ADR_CardBusCISPtr                   0x28
#define PCI_ADR_SubDevId_SubVendId              0x2c
#define PCI_ADR_ExpRomBaseAddr                  0x30
#define PCI_ADR_CapPtr                          0x34
#define PCI_ADR_Res3                            0x38
#define PCI_ADR_MaxLat_MinGnt_IrqPin_IrqLine    0x3c

// defines for Type 1 Configuration Space Header
#define PCI_ADR_Bar0							0x10
#define PCI_Adr_Bar1							0x14
#define PCI_ADR_SecLatTimer_BusNumbers			0x18
#define PCI_ADR_SecStatus_IOLimit_IOBase		0x1C
#define PCI_ADR_MemLimit_MemBase				0x20
#define PCI_ADR_PreMemLimit_PreMemBase			0x24

#define PCI_ADR_BrideCtrl_IrqPin_Irq_Line		0x3C

#define PCI_ADR_DPMHS_INT_EN0					0xFFF0
#define PCI_ADR_DPMHS_INT_STA0					0xFFE0
#define PCI_ADR_NetX_Sys_Stat					0xFFD8
#define PCI_ADR_Timer_StartValue				0xFFD4
#define PCI_ADR_Timer_Control					0xFFD0

#define NETX_SYS_STAT_CONTENT					0x030C0009

#define HBOOT_NETX4000RELAXED_DPM_VERSION_COOKIE      0x73413c08
#define HBOOT_NETX4000FULL_DPM_VERSION_COOKIE         0x84524c0b
#define HBOOT_NETX4000SMALL_DPM_VERSION_COOKIE        0x93615b0b

#define HBOOT_DPM_ID_LISTENING          0x4c42584e
#define HBOOT_DPM_ID_STOPPED            0x464e5552

#define DEVICEADDR_ONBOARD_PLX_BRIDGE       0x10000000U
#define DEVICEADDR_ONBOARD_PLX_BRIDGE_BAR0  0x30000000U
#define DEVICEADDR_CIFXM2_BAR0              0x376F0000U
#define DEVICEADDR_CIFXM2_IO_BAR0           0xd000U

#define BUS_ONBOARD_PLX_BRIDGE 0
#define BUS_CIFXM2           2

#endif  // __pci_plx__

