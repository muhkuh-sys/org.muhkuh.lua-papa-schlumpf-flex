local class = require 'pl.class'
local papaSchlumpfFlex = class()


function papaSchlumpfFlex:_init(tLog)
  self.tLog = tLog
  self.bit = require 'bit'
  self.pl = require 'pl.import_into'()
  local p = require 'papa_schlumpf'
  self.p = p
  self.tP = p.PapaSchlumpfFlex()

  -- This is the VID/PID of the papa schlumpf PCI->PCIe bridge.
  self.PCI_VID_PID_PLX_BRIDGE = 0x811210b5

  self.DMAACTION = {
    Write   = 0,
    Compare = 1,
    Dump    = 2
  }
  self.DMATYPE = {
    IO    = 0,
    Mem   = 1,
    Cfg0  = 2,
    Cfg1  = 3
  }
  self.PCIADR = {
    MSK_PCI_CONFIGURATION_ADDRESS_REGISTER = 0x000000ff,
    SRT_PCI_CONFIGURATION_ADDRESS_REGISTER = 0,
    MSK_PCI_CONFIGURATION_ADDRESS_FUNCTION = 0x00000700,
    SRT_PCI_CONFIGURATION_ADDRESS_FUNCTION = 8,
    MSK_PCI_CONFIGURATION_ADDRESS_DEVICE =   0x0000f800,
    SRT_PCI_CONFIGURATION_ADDRESS_DEVICE =   11,
    MSK_PCI_CONFIGURATION_ADDRESS_BUS =      0x00ff0000,
    SRT_PCI_CONFIGURATION_ADDRESS_BUS =      16
  }
  self.PCIINFO = {
    DevId_VendId =                 0x00,
    Status_Cmd =                   0x04,
    ClassCode_RevId =              0x08,
    HdrType_LatTimer_CacheSize =   0x0c,
    MemBar0 =                      0x10,
    MemBar1 =                      0x14,
    MemBar2 =                      0x18,
    MemBar3 =                      0x1c,
    MemBar4 =                      0x20,
    MemBar5 =                      0x24,
    CardBusCISPtr =                0x28,
    SubDevId_SubVendId =           0x2c,
    ExpRomBaseAddr =               0x30,
    CapPtr =                       0x34,
    Res3 =                         0x38,
    MaxLat_MinGnt_IrqPin_IrqLine = 0x3c,

    -- Defines for the type 1 header.
    Bar0 =                         0x10,
    Bar1 =                         0x14,
    SecLatTimer_BusNumbers =       0x18,
    SecStatus_IOLimit_IOBase =     0x1c,
    MemLimit_MemBase =             0x20,
    PreMemLimit_PreMemBase =       0x24,
    BrideCtrl_IrqPin_Irq_Line =    0x3c
  }
end



function papaSchlumpfFlex:CFG0ADR(ulAddress, ulOffset)
  local bit = self.bit

  return bit.bor(ulAddress, ulOffset)
end



function papaSchlumpfFlex:CFG1ADR(ulBus, ulDevice, ulFunction, ulRegister, ulParityFakeBit)
  local bit = self.bit
  local PCIADR = self.PCIADR

  local ulParityFake = 0
  if ulParityFakeBit~=nil then
    ulParityFake = bit.lshift(1, ulParityFakeBit)
  end

  return bit.bor(
    bit.band(bit.lshift(ulBus, PCIADR.SRT_PCI_CONFIGURATION_ADDRESS_BUS), PCIADR.MSK_PCI_CONFIGURATION_ADDRESS_BUS),
    bit.band(bit.lshift(ulDevice, PCIADR.SRT_PCI_CONFIGURATION_ADDRESS_DEVICE), PCIADR.MSK_PCI_CONFIGURATION_ADDRESS_DEVICE),
    bit.band(bit.lshift(ulFunction, PCIADR.SRT_PCI_CONFIGURATION_ADDRESS_FUNCTION), PCIADR.MSK_PCI_CONFIGURATION_ADDRESS_FUNCTION),
    bit.band(bit.lshift(ulRegister, PCIADR.SRT_PCI_CONFIGURATION_ADDRESS_REGISTER), PCIADR.MSK_PCI_CONFIGURATION_ADDRESS_REGISTER),
    ulParityFake
  )
end



function papaSchlumpfFlex:MEMADR(ulAddress, ulOffset)
  local bit = self.bit

  return bit.bor(ulAddress, ulOffset)
end



--- Connect to the papa schlumpf device.
-- Search for available papa schumpf devices. There must be only one device
-- or the function will throw an error. Connect to the device and print the
-- firmware version.
--
-- @return true on success or nil and error message otherwise.
function papaSchlumpfFlex:connect()
  local tLog = self.tLog
  local tP = self.tP

  local tResult, strError = tP:connect()
  if tResult~=true then
    tLog.error('Failed to connect: %s', strError)
  else
    local ulMajor, ulMinor, ulSub, strVcs = tP:getFirmwareVersion()
    if ulMajor==nil then
      tResult = nil
      strError = tostring(ulMinor)
      tLog.error('Failed to get the version information: %s', strError)
    else
      tLog.debug('Using firmware v%d.%d.%d %s.', ulMajor, ulMinor, ulSub, strVcs)
    end
  end

  return tResult, strError
end



--- Disconnect to the papa schlumpf device.
function papaSchlumpfFlex:disconnect()
  local tLog = self.tLog
  local tP = self.tP

  local tResult, strError = tP:disconnect()
  if tResult~=true then
    tLog.error('Failed to disconnect: %s', strError)
  end

  return tResult, strError
end



--- Reset PCI.
-- Reset the PCI core and the bus. This sends a reset to all devices too.
--
-- @param ulResetActiveToClock Delay between reset and clock activation in units of 100us. This parameter has a default of 500 ( = 50ms).
-- @param ulResetActiveDelayAfterClock Delay between clock ativation and reset deactivation in units of 100us. This parameter has a default of 1 ( = 100us).
-- @param ulBusIdleDelay Bus idle time after reset deactivation in units of 100us. This parameter has a default of 10000 ( = 1s).
--
-- @return true on success or nil and error message otherwise.
function papaSchlumpfFlex:resetPCI(ulResetActiveToClock, ulResetActiveDelayAfterClock, ulBusIdleDelay)
  -- Set the default values if a parameter is nil.
  ulResetActiveToClock = ulResetActiveToClock or 500
  ulResetActiveDelayAfterClock = ulResetActiveDelayAfterClock or 1
  ulBusIdleDelay = ulBusIdleDelay or 10000

  local tLog = self.tLog
  local tP = self.tP

  local tResult, strError = tP:resetPCI(ulResetActiveToClock, ulResetActiveDelayAfterClock, ulBusIdleDelay)
  if tResult~=true then
    tLog.error('Failed to reset the PCI bus and core: %s', strError)
  end

  return tResult, strError
end



function papaSchlumpfFlex:setPCIReset(ulResetState)
  local tLog = self.tLog
  local tP = self.tP

  local tResult, strError = tP:setPCIReset(ulResetState)
  if tResult~=true then
    tLog.error('Failed to set the PCI reset: %s', strError)
  end

  return tResult, strError
end



function papaSchlumpfFlex:setupNetx()
  local tLog = self.tLog
  local tP = self.tP

  local tResult, strError = tP:setupNetx()
  if tResult~=true then
    tLog.error('Failed to setup the netX PCI core: %s', strError)
  end

  return tResult, strError
end



function papaSchlumpfFlex:processDmaList(atDma)
  local tLog = self.tLog
  local tP = self.tP
  local tResult = true

  local DMAACTION = self.DMAACTION
  local DMATYPE = self.DMATYPE

  for uiLine, tDma in ipairs(atDma) do
    local tAction = tDma[1]
    local tType = tDma[2]
    local ulAddress = tDma[3]
    local ulValue = tDma[4]

    -- The results of the bit module are signed. Convert them to unsigned here.
    if ulAddress<0 then
      ulAddress = 0x100000000 + ulAddress
    end
    if ulValue<0 then
      ulValue = 0x100000000 + ulValue
    end

    if tAction~=DMAACTION.Write and tAction~=DMAACTION.Compare and tAction~=DMAACTION.Dump then
      tLog.error('Invalid action in DMA list entry %02d: %s', uiLine, tostring(tAction))
      tResult = nil
      break
    elseif tType~=DMATYPE.IO and tType~=DMATYPE.Mem and tType~=DMATYPE.Cfg0 and tType~=DMATYPE.Cfg1 then
      tLog.error('Invalid type in DMA list entry %02d: %s', uiLine, tostring(tType))
      tResult = nil
      break
    elseif type(ulAddress)~='number' then
      tLog.error('Invalid address in DMA list entry %02d: %s', uiLine, tostring(ulAddress))
      tResult = nil
      break
    elseif ulAddress<0x00000000 or ulAddress>0xffffffff then
      tLog.error('Invalid address in DMA list entry %02d, must be an unsigned 32 bit value: %s', uiLine, tostring(ulAddress))
      tResult = nil
      break
    elseif type(ulValue)~='number' then
      tLog.error('Invalid value in DMA list entry %d: %s', uiLine, tostring(ulValue))
      tResult = nil
      break
    elseif ulValue<0x00000000 or ulValue>0xffffffff then
      tLog.error('Invalid value in DMA list entry %02d, must be an unsigned 32 bit value: %s', uiLine, tostring(ulValue))
      tResult = nil
      break
    else
      local strError
      if tAction==DMAACTION.Write then
        if tType==DMATYPE.IO then
          tResult, strError = tP:ioWrite(ulAddress, ulValue)
        elseif tType==DMATYPE.Mem then
          tResult, strError = tP:memWrite(ulAddress, ulValue)
        elseif tType==DMATYPE.Cfg0 then
          tResult, strError = tP:cfg0Write(ulAddress, ulValue)
        elseif tType==DMATYPE.Cfg1 then
          tResult, strError = tP:cfg1Write(ulAddress, ulValue)
        end
        if tResult==nil then
          tLog.error('DMA list entry %02d failed to write: %s', uiLine, tostring(strError))
          break
        end
      else
        local ulReadValue
        if tType==DMATYPE.IO then
          ulReadValue, strError = tP:ioRead(ulAddress)
        elseif tType==DMATYPE.Mem then
          ulReadValue, strError = tP:memRead(ulAddress)
        elseif tType==DMATYPE.Cfg0 then
          ulReadValue, strError = tP:cfg0Read(ulAddress)
        elseif tType==DMATYPE.Cfg1 then
          ulReadValue, strError = tP:cfg1Read(ulAddress)
        end

        if ulReadValue==nil then
          tResult = nil
          tLog.error('DMA list entry %02d failed to read: %s', uiLine, tostring(strError))
          break
        else
          if tAction==DMAACTION.Compare then
            if ulReadValue==ulValue then
              tLog.debug('DMA list entry %02d compare is equal: 0x%08x', uiLine, ulReadValue)
            else
              tLog.error('DMA list entry %02d compare is not equal: expected 0x%08x, read 0x%08x', uiLine, ulValue, ulReadValue)
              tResult = false
              break
            end
          else
            tLog.debug('DMA list entry %02d dump: 0x%08x', uiLine, ulReadValue)
          end
        end
      end
    end
  end

  return tResult
end



--- Find the root bridge.
-- Find the PCI->PCIe bridge on the papa schlumpf board.
--
-- @return the address of the root bridge on success or nil and error message otherwise.
function papaSchlumpfFlex:findRootBridge()
  local tLog = self.tLog
  local tP = self.tP
  local tResult = true
  local strError

  -- Scan for devices.
  local ulBridgeAddress
  local ulAddress = 0x00010000
  repeat
    local ulVidPid, strError = tP:cfg0Read(ulAddress)
    -- Ignore failed transfers. This means that no device is there.
    if ulVidPid~=nil and ulVidPid~=0x00000000 then
      if ulVidPid==self.PCI_VID_PID_PLX_BRIDGE then
        ulBridgeAddress = ulAddress
        tLog.debug('Found papa schlumpf bridge at 0x%08x.', ulAddress)
      else
        local ulVid = math.floor(ulVidPid / 0x00010000)
        local ulPid = ulVidPid - 0x00010000*ulVid
        tLog.error('Found unexpected device with VID=%04x and PID=%04x at address 0x%08x.', ulVid, ulPid, ulAddress)
        tResult = false
      end
    end

    ulAddress = ulAddress * 2
  until ulAddress>0x10000000

  if ulBridgeAddress==nil then
    strError = 'No papa schlumpf device found.'
    tResult = false
  elseif tResult~=true then
    strError = 'Unexpected devices found.'
  else
    tResult = ulBridgeAddress
  end

  return tResult
end



function papaSchlumpfFlex:ioRead(ulAddress)
  local tData, strError = self.tP:ioRead(ulAddress)
  if tData==nil then
    error(string.format('ioRead(0x%08x) failed: %s', ulAddress, strError))
  end
  return tData
end



function papaSchlumpfFlex:memRead(ulAddress)
  local tData, strError = self.tP:memRead(ulAddress)
  if tData==nil then
    error(string.format('memRead(0x%08x) failed: %s', ulAddress, strError))
  end
  return tData
end



function papaSchlumpfFlex:memReadArea(ulAddress, ulSize)
  local tData, strError = self.tP:memReadArea(ulAddress, ulSize)
  if tData==nil then
    error(string.format('memReadArea(0x%08x, %d) failed: %s', ulAddress, ulSize, strError))
  end
  return tData
end



function papaSchlumpfFlex:cfg0Read(ulAddress)
  local tData, strError = self.tP:cfg0Read(ulAddress)
  if tData==nil then
    error(string.format('cfg0Read(0x%08x) failed: %s', ulAddress, strError))
  end
  return tData
end



function papaSchlumpfFlex:cfg1Read(ulAddress)
  local tData, strError = self.tP:cfg1Read(ulAddress)
  if tData==nil then
    error(string.format('cfg1Read(0x%08x) failed: %s', ulAddress, strError))
  end
  return tData
end



function papaSchlumpfFlex:ioWrite(ulAddress, ulData)
  local tResult, strError = self.tP:ioWrite(ulAddress, ulData)
  if tResult~=true then
    error(string.format('ioWrite(0x%08x, 0x08x) failed: %s', ulAddress, ulData, strError))
  end
end



function papaSchlumpfFlex:memWrite(ulAddress, ulData)
  local tResult, strError = self.tP:memWrite(ulAddress, ulData)
  if tResult~=true then
    error(string.format('memWrite(0x%08x, 0x08x) failed: %s', ulAddress, ulData, strError))
  end
end



function papaSchlumpfFlex:memWriteArea(ulAddress, strData)
  local tResult, strError = self.tP:memWriteArea(ulAddress, strData)
  if tResult~=true then
    error(string.format('memWriteArea(0x%08x, ...) failed: %s', ulAddress, strError))
  end
end



function papaSchlumpfFlex:cfg0Write(ulAddress, ulData)
  local tResult, strError = self.tP:cfg0Write(ulAddress, ulData)
  if tResult~=true then
    error(string.format('cfg0Write(0x%08x, 0x08x) failed: %s', ulAddress, ulData, strError))
  end
end



function papaSchlumpfFlex:cfg1Write(ulAddress, ulData)
  local tResult, strError = self.tP:cfg1Write(ulAddress, ulData)
  if tResult~=true then
    error(string.format('cfg1Write(0x%08x, 0x08x) failed: %s', ulAddress, ulData, strError))
  end
end


return papaSchlumpfFlex
