
#ifndef __PCI__
#define __PCI__

//-------------------------------------

// reset_ctrl register
#define reset_ctrl_RSTOUTn 0x02000000
#define reset_ctrl_EN_RSTOUTn 0x04000000

// uFreqValue = freq*2^32 / 200MHz falsch!
// uFreqValue = freq*200MHz / 2^32
#define clockOut_Off 0
#define clockOut_FreqMHzToValue(freq) (freq*21474836.48 + 0.5)

//-------------------------------------

void dpm_init_registers(void);
void dpm_deinit_registers(void);

void delay100US(unsigned int uDelay100Us);

void clockOutConfig(unsigned int uFreqValue);

void pciInitGlobals(void);
void pciModeEnable(void);
void pciModeDisable(void);

int pciResetAndInit(unsigned int uRstActiveToClock, unsigned int uRstActiveDelayAfterClock, unsigned int uBusIdleDelay);
void pciReset(unsigned int uRstActiveToClock, unsigned int uRstActiveDelayAfterClock, unsigned int uBusIdleDelay);

int pciDma_Ch0(unsigned int uPciStartAdr, volatile unsigned long *pulNetxAdr, unsigned int uDmaCtrl);

int pciDeviceScan(unsigned long *pulDevData, volatile unsigned long *pulDmaArea);

int pciReadReq(unsigned int uWrCtrl, unsigned int *uData);
int pciWriteReq(unsigned int uWrCtrl, unsigned int uData);

void pciSetMemoryWindow0(unsigned long ulStartAdr, unsigned long ulLength);
void pciSetBurstLength(unsigned long size);
unsigned long pciGetDeviceAddr(unsigned long ulPciID, unsigned long *pulDevData);

int pciNetXDeviceConfigRead(void);
void pciArbConfig(void);

int pciDma_CfgRead(unsigned int uDeviceAdr, volatile unsigned long *pulNetxAdr, unsigned int uDwords);
int pciDma_CfgWrite(unsigned int uDeviceAdr, volatile unsigned long *pulNetxAdr, unsigned int uDwords);

int pciDma_MemRead(unsigned int uDeviceAdr, volatile unsigned long *pulNetxAdr, unsigned int uDwords);
int pciDma_MemWrite(unsigned int uDeviceAdr, volatile unsigned long *pulNetxAdr, unsigned int uDwords);

unsigned long swapBit0_Bit30(unsigned long ulDeviceAdr);
void swapBi0_Bit30_array(volatile unsigned long *aulData, unsigned long ulCount);

int pciDma_CfgRead_Type1(unsigned int uDeviceAdr, volatile unsigned long *pulNetxAdr, unsigned int uDwords);
int pciDma_CfgWrite_Type1(unsigned int uDeviceAdr, volatile unsigned long *pulNetxAdr, unsigned int uDwords);

int pciDma_TestCBE(unsigned int uDeviceAdr, unsigned int uDeviceOffs, volatile unsigned long *pulNetxAdr, unsigned int uDwords);


#define MSK_DPMAS_NETX_DMA_CTRL_DONE                                     0x80000000
#define MSK_DPMAS_NETX_DMA_CTRL_STATUS                                   0x70000000
#define MSK_DPMAS_NETX_DMA_CTRL_START                                    0x08000000
#define MSK_DPMAS_NETX_DMA_CTRL_ACTIVE                                   0x04000000
#define MSK_DPMAS_NETX_DMA_CTRL_INT_REQ                                  0x02000000
#define MSK_DPMAS_NETX_DMA_CTRL_MBX_REQ                                  0x01000000
#define MSK_DPMAS_NETX_DMA_CTRL_DMA_TYPE                                 0x00E00000
#define MSK_DPMAS_NETX_DMA_CTRL_DIRECTION                                0x00100000
#define MSK_DPMAS_NETX_DMA_CTRL_TRANSFER_LENGTH                          0x000FFFFC

#define SRT_DPMAS_NETX_DMA_CTRL_STATUS                                   28
#define SRT_DPMAS_NETX_DMA_CTRL_DMA_TYPE                                 21
#define SRT_DPMAS_NETX_DMA_CTRL_DIRECTION                                20
#define SRT_DPMAS_NETX_DMA_CTRL_TRANSFER_LENGTH                          2

#define VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Input_Output_Cycle              1
#define VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Memory_Cycle                    3
#define VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Configuration_Cycle             5
#define VAL_DPMAS_NETX_DMA_CTRL_DMA_TYPE_Memory_Read_Multiple            6

#define VAL_DPMAS_NETX_DMA_CTRL_STATUS_DMA_idle_finished                 0
#define VAL_DPMAS_NETX_DMA_CTRL_STATUS_DMA_target_device_select_timeout  1
#define VAL_DPMAS_NETX_DMA_CTRL_STATUS_DMA_target_nTRDY_timeout          2
#define VAL_DPMAS_NETX_DMA_CTRL_STATUS_DMA_target_abort                  3
#define VAL_DPMAS_NETX_DMA_CTRL_STATUS_DMA_transfer_parity_error         4
#define VAL_DPMAS_NETX_DMA_CTRL_STATUS_mbx_error                         7

#define VAL_DPMAS_NETX_DMA_CTRL_DIRECTION_host_to_netx                   0
#define VAL_DPMAS_NETX_DMA_CTRL_DIRECTION_netx_to_host                   1

#define RESULT_OK                                                        0
#define RESULT_DMA_ERROR                                                 1
#define RESULT_DMA_PARITY_ERROR                                          2
#define RESULT_READBACK_VERIFY_ERROR                                     3

#define MSK_BIT0                                                         0x00000001U
#define MSK_BIT30                                                        0x40000000U

#define SRT_BIT0                                                         0U
#define SRT_BIT30                                                        30U

#define SRT_IDSEL                                                        28U


#define SRT_PCI_HEADER_TYPE 0

/* The address for a configuration access is split into several fields. */
#define MSK_PCI_CONFIGURATION_ADDRESS_REGISTER   0x000000ffU
#define SRT_PCI_CONFIGURATION_ADDRESS_REGISTER   0
#define MSK_PCI_CONFIGURATION_ADDRESS_FUNCTION   0x00000700U
#define SRT_PCI_CONFIGURATION_ADDRESS_FUNCTION   8
#define MSK_PCI_CONFIGURATION_ADDRESS_DEVICE     0x0000f800U
#define SRT_PCI_CONFIGURATION_ADDRESS_DEVICE     11
#define MSK_PCI_CONFIGURATION_ADDRESS_BUS        0x00ff0000U
#define SRT_PCI_CONFIGURATION_ADDRESS_BUS        16

/* Combine all configuration address parts. */
#define CFG1ADR(BUS,DEVICE,FUNCTION,REGISTER) (((BUS<<SRT_PCI_CONFIGURATION_ADDRESS_BUS)&MSK_PCI_CONFIGURATION_ADDRESS_BUS) | ((DEVICE<<SRT_PCI_CONFIGURATION_ADDRESS_DEVICE)&MSK_PCI_CONFIGURATION_ADDRESS_DEVICE) | ((FUNCTION<<SRT_PCI_CONFIGURATION_ADDRESS_FUNCTION)&MSK_PCI_CONFIGURATION_ADDRESS_FUNCTION) | ((REGISTER<<SRT_PCI_CONFIGURATION_ADDRESS_REGISTER)&MSK_PCI_CONFIGURATION_ADDRESS_REGISTER))

#define CFG0ADR(ADDRESS,OFFSET) (ADDRESS|OFFSET)

#define MEMADR(ADDRESS,OFFSET) (ADDRESS|OFFSET)


//-------------------------------------


/* The action of a DMA list entry.
 * The action describes the DMA type (read/write) and what should be done with any received data (compare or dump).
 */
typedef enum DMAACTION_ENUM
{
	DMAACTION_Write    = 0,
	DMAACTION_Compare  = 1,
	DMAACTION_Dump     = 2
} DMAACTION_T;



typedef enum DMATYPE_ENUM
{
	DMATYPE_IO    = 0,
	DMATYPE_Mem   = 1,
	DMATYPE_Cfg0  = 2,
	DMATYPE_Cfg1  = 3
} DMATYPE_T;



/* This is a complete entry in a DMA list. */
typedef struct DMALIST_STRUCT
{
	DMAACTION_T tAct;
	DMATYPE_T tTyp;
	unsigned long ulAddress;
	unsigned long ulData;
} DMALIST_T;


/* Combine the DMA type with the address. */
#define DMA_CFG0(ADDRESS,OFFSET)                  DMATYPE_Cfg0, CFG0ADR(ADDRESS,OFFSET)

#define DMA_CFG1(BUS,DEVICE,FUNCTION,REGISTER)    DMATYPE_Cfg1, CFG1ADR(BUS,DEVICE,FUNCTION,REGISTER)
#define DMA_MEM(ADDRESS,OFFSET)                   DMATYPE_Mem,  MEMADR(ADDRESS,OFFSET)


int dmalist_process(const DMALIST_T *ptDmaList, unsigned int sizDmaList, const char *pcName);

int io_register_read_08(unsigned int ulAddress, unsigned char *pucData);
int io_register_read_32(unsigned int ulAddress, unsigned long *pulData);
int io_register_write_32(unsigned int ulAddress, unsigned long ulData);

#endif  /* __PCI__ */
