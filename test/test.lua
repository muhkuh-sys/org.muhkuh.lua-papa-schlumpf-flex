require 'muhkuh_cli_init'

local bit = require 'bit'
local pl = require 'pl.import_into'()

-- Set the log level.
local strLogLevel = 'debug'

local tLogWriter = require 'log.writer.console.color'.new()
local tLog = require "log".new(
  strLogLevel,
  tLogWriter,
  require "log.formatter.format".new()
)

tLog.info('PCIe test')



local DEVICEADDR_ONBOARD_PLX_BRIDGE_BAR0 = 0x30000000
local DEVICEADDR_CIFXM2_BAR0 =             0x376F0000
local DEVICEADDR_CIFXM2_IO_BAR0 =          0xd000

local PCI_ID_CIFXM2 = 0x009015cf

local BUS_CIFXM2 = 2




local function netx90_activate_spm_bootmode()
  local openocd = require 'luaopenocd'

  -- Read the TCL script.
  local strScriptFileName = 'netX90_NXJTAG-4000-USB_activate_SPM.tcl'
  local strFileName = pl.path.exists(strScriptFileName)
  if strFileName~=strScriptFileName then
    tLog.error('The TCL script "%s" does not exist.', strScriptFileName)
    error('Failed to load the TCL script.')
  end
  local strScript = pl.file.read(strFileName)

  -- Get the number of retries.
  local ulRetries = 1

  -- Define the callback function for openocd.
  local fnCallback = function(strMessage)
    tLog.debug(strMessage)
  end
  local tOpenOCD = openocd.luaopenocd(fnCallback)

  local ulRetryCnt = ulRetries
  local fOK = false
  repeat
    tLog.debug('Initialize OpenOCD.')
    tOpenOCD:initialize()

    local strResult
    local iResult = tOpenOCD:run(strScript)
    if iResult~=0 then
      error('Failed to execute the script.')
    else
      strResult = tOpenOCD:get_result()
      tLog.info('Script result: %s', strResult)
      if strResult=='0' then
        fOK = true
        tOpenOCD:run('sleep 500')
      else
        tLog.debug('The script result is not "0".')
        if ulRetryCnt==0 then
          break
        else
          ulRetryCnt = ulRetryCnt - 1
          tOpenOCD:run('sleep 500')
        end
      end
    end

    tLog.debug('Uninitialize OpenOCD.')
    tOpenOCD:uninit()
  until fOK==true

  return fOK
end



local ASIX_SPI_REG = {
  SPICMR =  0x00,  -- Control Master Register
  SPICSS =  0x01,  -- Clock Source Select 
  SPIBRR =  0x04,  -- Baud Rate Register
  SPIDS =   0x05,  -- Delay before SCLK
  SPIDT =   0x06,  -- Delay transmission
  SDAOF =   0x07,  -- Delay after OPCode field
  STOF0 =   0x08,  -- OPCode fields 0-7
  STOF1 =   0x09,
  STOF2 =   0x0A,
  STOF3 =   0x0B,
  STOF4 =   0x0C,
  STOF5 =   0x0D,
  STOF6 =   0x0E,
  STOF7 =   0x0F,
  SDFL0 =   0x10,  -- DMA Fragment length 0-1
  SDFL1 =   0x11,
  SPISSOL = 0x12,  -- Slave Select Register and OPCode length
  SDCR =    0x13,  -- DMA Command Register
  SPIMISR = 0x14   -- Master interrupt status register
}

local ASIX_SPI_REG_SPICMR = {
  SSP =    0x01,
  CPHA =   0x02,
  CPOL =   0x04,
  LSB =    0x08,
  SPIMEN = 0x10,
  ASS =    0x20,
  SWE =    0x40,
  SSOE =   0x80
}

local ASIX_SPI_REG_SPICSS = {
  CLK125MHZ =  0x00,
  CLK100MHZ =  0x01,
  DIVEN =      0x04,
  SPICKS_MSK = 0x03
}

local ASIX_SPI_REG_SDCR = {
  ETDMA =   0x01,  -- Transmit DMA
  ERDMA =   0x02,  -- Receive DMA
  OPCFE =   0x04,  -- OPCode enable field
  DMA_GO =  0x08,
  FBT =     0x10,
  CSS =     0x20,
  STCIE =   0x40,  -- Interrupt enable
  STERRIE = 0x80   -- Interrupt enable on error
}

local ASIX_SPI_REG_SPIMISR = {
  STC =   0x01, -- SPI Tansceiver Complete Flag
  STERR = 0x02  -- SPI Tansceiver Error Indication
}


local function spm_access(tP)
  -- Set the SPI control master register.
  local ulValue = bit.bor(
    ASIX_SPI_REG_SPICMR.CPHA,   -- Set mode 3.
    ASIX_SPI_REG_SPICMR.CPOL,   -- Set mode 3.
    ASIX_SPI_REG_SPICMR.SPIMEN, -- Enable the master controller.
    ASIX_SPI_REG_SPICMR.SSOE    -- Drive the chip select pins.
  )
  local ulCombined = ulValue
  -- Set the clock source select register.
  ulValue  = ASIX_SPI_REG_SPICSS.CLK100MHZ  -- Use 100MHz. */
  ulCombined = bit.bor(ulCombined, bit.lshift(ulValue, 8))
  local tResult, strError = tP.tP:ioWrite(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_REG.SPICMR, ulCombined)
  assert(tResult)

  ulCombined = bit.bor(
    -- Set the clock divider to 1MHz (100MHz / 100).
    100,
    -- No delay after clip select activation.
    bit.lshift(0, 8),
    -- No delay time between chip selections.
    bit.lshift(0, 16),
    -- No delay after the opcode.
    bit.lshift(0, 24)
  )
  tResult, strError = tP.tP:ioWrite(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_REG.SPIBRR, ulCombined)
  assert(tResult)

  -- Set the SPI control master register.
  ulValue = bit.bor(
    ASIX_SPI_REG_SPICMR.CPHA,   -- Set mode 3.
    ASIX_SPI_REG_SPICMR.CPOL,   -- Set mode 3.
    ASIX_SPI_REG_SPICMR.SPIMEN, -- Enable the master controller.
    ASIX_SPI_REG_SPICMR.SSOE    -- Drive the chip select pins.
  )
  ulCombined = ulValue
  -- Set the clock source select register.
  ulValue = bit.bor(
    ASIX_SPI_REG_SPICSS.CLK100MHZ, -- Use 100MHz.
    ASIX_SPI_REG_SPICSS.DIVEN      -- Activate the clock divider.
  )
  ulCombined = bit.bor(ulCombined, bit.lshift(ulValue, 8))
  tResult, strError = tP.tP:ioWrite(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_REG.SPICMR, ulCombined)
  assert(tResult)

  -- Set the data to transfer.
  ulValue = 0x00000180
  tResult, strError = tP.tP:ioWrite(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_REG.STOF0, ulValue)
  assert(tResult)
  ulValue = 0x00000000
  tResult, strError = tP.tP:ioWrite(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_REG.STOF4, ulValue)
  assert(tResult)

  -- Activate the chip select and set the transfer length.
  ulValue = bit.bor(
    6, -- Select slave 0.
    bit.lshift(7, 4)  -- Transfer 8 bytes.
  )
  ulCombined = bit.lshift(ulValue, 16)
  tResult, strError = tP.tP:ioWrite(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_REG.SDFL0, ulCombined)
  assert(tResult)

  -- Start the transfer.
  -- NOTE: this uses the old ulCombined value.
  ulValue = bit.bor(
    ASIX_SPI_REG_SDCR.OPCFE,
    ASIX_SPI_REG_SDCR.DMA_GO
  )
  ulCombined = bit.bor(ulCombined, bit.lshift(ulValue, 24))
  tResult, strError = tP.tP:ioWrite(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_REG.SDFL0, ulCombined)
  assert(tResult)

  -- Wait for transfer finish.
  local uiRetries = 65536
  repeat
    local tReadResult, strError = tP.tP:ioRead(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_REG.SPIMISR)
    -- Ignore failed read operations. This seems to happen.
    if tReadResult==nil then
      tReadResult = 0
    end

    uiRetries = uiRetries - 1
    if uiRetries<=0 then
      tResult = nil
      break
    end
  until bit.band(tReadResult, ASIX_SPI_REG_SPIMISR.STC)~=0

  if tResult==true then
    -- Read the data.
    tResult, strError = tP.tP:ioRead(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_REG.STOF0)
    assert(tResult)
    tResult, strError = tP.tP:ioRead(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_REG.STOF4)
    assert(tResult)

    -- Confirm the IRQ.
    ulValue = ASIX_SPI_REG_SPIMISR.STC
    tP.tP:ioWrite(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_REG.SPIMISR, ulValue)
  end

  -- Deactivate the chip select.
  ulValue = 7  -- Select no slave.
  ulCombined = bit.lshift(ulValue, 16)
  tP.tP:ioWrite(DEVICEADDR_CIFXM2_IO_BAR0+ASIX_SPI_REG.SDFL0, ulCombined)

  return tResult
end

 

local tP = require 'papa_schlumpf_flex'(tLog)
local tResult = tP:connect()
if tResult==true then
  assert(tP:resetPCI())
  local ulAddressRootBridge = tP:findRootBridge()
  if ulAddressRootBridge==nil then
    error('Root bridge not found.')
  end

  local DMAACTION = tP.DMAACTION
  local DMATYPE = tP.DMATYPE
  local PCIINFO = tP.PCIINFO

  tResult = tP:processDmaList{
    -- 01: Write 0xffffffff to the memory bar 0 register.
    { DMAACTION.Write,   DMATYPE.Cfg0, tP:CFG0ADR(ulAddressRootBridge, PCIINFO.MemBar0),                    0xffffffff },

    -- 02: The memory bar 0 register must report a size of 64K.
    { DMAACTION.Compare, DMATYPE.Cfg0, tP:CFG0ADR(ulAddressRootBridge, PCIINFO.MemBar0),                    0xffff000c },

    -- 03: Clear the "Master Data Parity Error" bit in the status register.
    --     Activate i/o access, memory space, bus master.
    { DMAACTION.Write,   DMATYPE.Cfg0, tP:CFG0ADR(ulAddressRootBridge, PCIINFO.Status_Cmd),                 0x01000117 },

    -- 04: Set the cache line size and the bus latency timer.
    --
    --     The cache line size is at offset 0x0c, bits 0-7. Bit 8 seems to be reserved.
    --     Set the cache line size to 0 DWORDs.
    --
    --     Set a bus latency of 64.
    { DMAACTION.Write,   DMATYPE.Cfg0, tP:CFG0ADR(ulAddressRootBridge, PCIINFO.HdrType_LatTimer_CacheSize), 0x00004080 },

    -- 05: Set the base address of the bridge window.
    { DMAACTION.Write,   DMATYPE.Cfg0, tP:CFG0ADR(ulAddressRootBridge, PCIINFO.Bar0),                       bit.bor(DEVICEADDR_ONBOARD_PLX_BRIDGE_BAR0, 0x0000000c) },

    -- 06: Set the bus numbers.
    --     Set the primary bus to 1.
    --     Set the secondary bus to 2.
    --     Set the number of the highest bus behind the PEX to 7.
    { DMAACTION.Write,   DMATYPE.Cfg0, tP:CFG0ADR(ulAddressRootBridge, PCIINFO.SecLatTimer_BusNumbers),     0x40070201 },

    -- 07: Set the IO base and limit. Clear the SecStatus.
    --     Set the IO base and limit to 0xd to enable I/O access in the area 0xd000-0xdfff.
    { DMAACTION.Write,   DMATYPE.Cfg0, tP:CFG0ADR(ulAddressRootBridge, PCIINFO.SecStatus_IOLimit_IOBase),   bit.bor(bit.lshift(0xf900, 16), bit.band(DEVICEADDR_CIFXM2_IO_BAR0, 0xf000), bit.rshift(bit.band(DEVICEADDR_CIFXM2_IO_BAR0, 0xf000), 8))},

    -- 08: Set the memory base and limit.
    --
    --     Set the memory base to 0x37600000 and the limit to 0x376FFFFF.
    --     This forwards all accesses in the configured area to the secondary bus.
    { DMAACTION.Write,   DMATYPE.Cfg0, tP:CFG0ADR(ulAddressRootBridge, PCIINFO.MemLimit_MemBase),           0x37603760 },

    -- 09: Set the prefetchable memory base and limit.
    --
    --     Set the prefetchable memory base to 0x37600000 and the limit to 0x376FFFFF.
    --     This forwards all prefetchable accesses in the configured area to the secondary bus.
    { DMAACTION.Write,   DMATYPE.Cfg0, tP:CFG0ADR(ulAddressRootBridge, PCIINFO.PreMemLimit_PreMemBase),     0x37603760 },

    -- 10: Set the interrupt lines and the bridge control.
    --
    --     Set the interrupt line to 16.
    --     Set the interrupt pin to 1.
    --
    --     Enable SERR, but this has no effect according to the manual.
    { DMAACTION.Write,   DMATYPE.Cfg0, tP:CFG0ADR(ulAddressRootBridge, PCIINFO.BrideCtrl_IrqPin_Irq_Line),  0x00020110 }
  }
  assert(tResult, "Failed to setup the PCI bridge.")

  local ulCifxM2VidPid, strError = tP.tP:cfg1Read(tP:CFG1ADR(BUS_CIFXM2, 0, 0, PCIINFO.DevId_VendId))
  if ulCifxM2VidPid==nil then
    tLog.error('Failed to read the VID/PID from the cifX M2 card: %s', tostring(strError))
  elseif ulCifxM2VidPid~=PCI_ID_CIFXM2 then
    tLog.error('No cifXM2 found. Expected ID: 0x08x, found: 0x%08x', PCI_ID_CIFXM2, ulCifxM2VidPid)
  else
    tLog.info('Found cifX M2 card with programmed EEPROM.')

    tResult = tP:processDmaList{
      -- 01: Check the subdevice and subvendor IDs.
      { DMAACTION.Compare, DMATYPE.Cfg1, tP:CFG1ADR(BUS_CIFXM2,0,0,PCIINFO.SubDevId_SubVendId),          0x100215cf },

      -- 02: The M2 card should have a function #3.
      { DMAACTION.Compare, DMATYPE.Cfg1, tP:CFG1ADR(BUS_CIFXM2,0,3,PCIINFO.DevId_VendId),                PCI_ID_CIFXM2 },
      -- 03: Check the subdevice and subvendor IDs of function #3.
      { DMAACTION.Compare, DMATYPE.Cfg1, tP:CFG1ADR(BUS_CIFXM2,0,3,PCIINFO.SubDevId_SubVendId),          0x600115cf },

      -- 04: Write 0xffffffff to the memory bar 0 register.
      { DMAACTION.Write,   DMATYPE.Cfg1, tP:CFG1ADR(BUS_CIFXM2,0,3,PCIINFO.MemBar0),                     0xffffffff },
      -- 05: The bar should report a size of 128 bytes.
      { DMAACTION.Compare, DMATYPE.Cfg1, tP:CFG1ADR(BUS_CIFXM2,0,3,PCIINFO.MemBar0),                     0xffffff81 },
      -- 06: Assign an address to bar 0.
      { DMAACTION.Write,   DMATYPE.Cfg1, tP:CFG1ADR(BUS_CIFXM2,0,3,PCIINFO.MemBar0),                     bit.bor(DEVICEADDR_CIFXM2_IO_BAR0, 1) },
      -- 07: Show the register contents.
      { DMAACTION.Compare, DMATYPE.Cfg1, tP:CFG1ADR(BUS_CIFXM2,0,3,PCIINFO.MemBar0),                     bit.bor(DEVICEADDR_CIFXM2_IO_BAR0, 1) },

      -- TODO: activate BAR1.
--      { DMAACTION.Write,   DMA_CFG1(BUS_CIFXM2,0,3,PCI_ADR_MemBar1)|0x01000000,          0xffffffff },
--      { DMAACTION.Dump,    DMA_CFG1(BUS_CIFXM2,0,3,PCI_ADR_MemBar1),                     0 },

      -- 08: Activate i/o access, memory space and bus master.
      --     NOTE: we need a parity fix here.
      { DMAACTION.Write,   DMATYPE.Cfg1, tP:CFG1ADR(BUS_CIFXM2,0,3,PCIINFO.Status_Cmd)+0x01000000,       0x00000147 },
    
      -- 09: try to access the IO space.
      { DMAACTION.Dump,    DMATYPE.IO,   DEVICEADDR_CIFXM2_IO_BAR0, 0 }
    }
    assert(tResult, "Failed to setup the cifX M2 card.")

    -- Now activate the SPM bootmode.
    tResult = netx90_activate_spm_bootmode()
    assert(tResult, "Failed to activate the SPM boot mode.")

    -- Try to access the SPM.
    -- NOTE: 2 dummy accesses are needed before the real data appears
    spm_access(tP)
    spm_access(tP)
    local ulDpmData = spm_access(tP)
    tLog.debug('Read 0x%08x.', ulDpmData)
    -- Compare the DPM data with the NXBL ID.
    if ulDpmData==0x4c42584e then
      tLog.info('Found the NXBL ID.')
    else
      tLog.error('Expected 0x4c42584e, but received 0x%08x.', ulDpmData)
    end
  end

  tP:disconnect()
end
print('End')
