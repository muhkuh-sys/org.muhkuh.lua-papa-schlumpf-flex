local class = require 'pl.class'
local Plugin = class()

function Plugin:_init(strPluginName, strPluginTyp, tLogWriter, strLogLevel, tPapaSchlumpf, ulPciBaseAddress)
  self.pl = require'pl.import_into'()

  self.strPluginName = strPluginName
  self.strPluginTyp = strPluginTyp

  self.tPapaSchlumpf = tPapaSchlumpf
  self.ulPciBaseAddress = ulPciBaseAddress

  -- Create a new log target for this plugin.
  local tLogWriterPlugin = require 'log.writer.prefix'.new(
    string.format('[%s] ', strPluginName),
    tLogWriter
  )
  self.tLog = require 'log'.new(
    strLogLevel,
    tLogWriterPlugin,
    require 'log.formatter.format'.new()
  )

  local vstruct = require 'vstruct'

  self.tStructureMailboxInfo = vstruct.compile([[
    aucMagic:s16
    ulVersion:u4
    ulControlRxOffset:u4
    ulControlTxOffset:u4
    ulBufferRxOffset:u4
    ulBufferTxOffset:u4
    ulBufferRxSize:u4
    ulBufferTxSize:u4
    ulChipTyp:u4
    aulReserved:s16
  ]])
  self.sizMailboxInfo = 64
  self.strMailboxMagic = 'Muhkuh DPM Data '

  self.tStructureMailboxControl = vstruct.compile([[
    ulReqCnt:u4
    ulAckCnt:u4
    ulDataSize:u4
    ulReserved0c:u4
  ]])
  self.sizMailboxControl = 16

  self.tStructurePacketReadWrite = vstruct.compile([[
    ucType:u1
    ulAddress:u4
  ]])
  self.sizPacketReadWrite = 5

  self.tStructurePacketReadArea = vstruct.compile([[
    ucType:u1
    ulAddress:u4
    usSize:u2
  ]])
  self.sizPacketReadArea = 7

  self.tStructurePacketCall = vstruct.compile([[
    ucType:u1
    ulAddress:u4
    ulR0:u4
  ]])
  self.sizPacketCall = 9

  self.MONITOR_PACKET_TYP = {
    Command_Read08        = 0x00,
    Command_Read16        = 0x01,
    Command_Read32        = 0x02,
    Command_Read64        = 0x03,
    Command_ReadArea      = 0x04,
    Command_Write08       = 0x05,
    Command_Write16       = 0x06,
    Command_Write32       = 0x07,
    Command_Write64       = 0x08,
    Command_WriteArea     = 0x09,
    Command_Call          = 0x0a,
    ACK                   = 0x0b,
    Status                = 0x0c,
    Read_Data             = 0x0d,
    Call_Data             = 0x0e,
    Call_Cancel           = 0x0f,
    MagicData             = 0x4d,
    Command_Magic         = 0xff
  }

  self.MONITOR_STATUS = {
    Ok                        = 0x00,
    Call_Finished             = 0x01,
    InvalidCommand            = 0x02,
    InvalidPacketSize         = 0x03,
    InvalidSizeParameter      = 0x04,
    CommandInProgress         = 0x05
  }

  local ROMLOADER_CHIPTYP = {
    UNKNOWN              = 0,
    NETX500              = 1,
    NETX100              = 2,
    NETX50               = 3,
    NETX5                = 4,
    NETX10               = 5,
    NETX56               = 6,
    NETX56B              = 7,
    NETX4000_RELAXED     = 8,
    RESERVED9            = 9,
    NETX90_MPW           = 10,
    NETX4000_FULL        = 11,
    NETX4100_SMALL       = 12,
    NETX90               = 13,
    NETX90B              = 14,
    NETIOLA              = 15,
    NETIOLB              = 16
  }

  self.ROMLOADER_CHIPTYP = ROMLOADER_CHIPTYP

  local tChiptypName = {
    [0] = nil,
    [1] = "netX500",
    [2] = "netX100",
    [3] = "netX50",
    [4] = nil,
    [5] = "netX10",
    [6] = "netX51/52 Step A",
    [7] = "netX51/52 Step B",
    [8] = "netX4000 RLXD",
    [9] = nil,
    [10] = "netX90MPW",
    [11] = "netX4000 Full",
    [12] = "netX4100 Small",
    [13] = "netX90 Rev0",
    [14] = "netX90 Rev1",
    [15] = "netIOL MPW",
    [16] = "netIOL Rev0",
  }
  self.tChiptypName = tChiptypName

  -- Create a lookup table.
  local atIdToRomloaderChipTyp = {}
  for strName, ulId in pairs(ROMLOADER_CHIPTYP) do
    atIdToRomloaderChipTyp[ulId] = strName
  end
  self.atIdToRomloaderChipTyp = atIdToRomloaderChipTyp

  self.ulControlRxAddress = nil
  self.ulControlTxAddress = nil
  self.ulBufferRxAddress = nil
  self.ulBufferTxAddress = nil
  self.ulBufferRxSize = nil
  self.ulBufferTxSize = nil
  self.ulChipTyp = nil
  self.fIsConnected = false

  self.strPacketStart = string.char(0x2a)
end


function Plugin:GetName()
  return self.strPluginName
end


function Plugin:GetTyp()
  return self.strPluginTyp
end


function Plugin:GetLocation()
  -- TODO: return the USB path.
end


function Plugin:GetChiptypName(tChiptyp)
  local tChiptypName = self.tChiptypName

  local strChiptyp = tChiptypName[tChiptyp]
  if strChiptyp == nil then
    -- set chip name with unknown name
    strChiptyp = "unknown chip"
  end

  return strChiptyp
end

function Plugin:GetChiptyp()
  return self.ulChipTyp
end

function Plugin:IsConnected()
  return self.fIsConnected
end


function Plugin:Disconnect()
  local tLog = self.tLog

  if self.fIsConnected~=true then
    tLog.debug('Ignoring disconnect request. Already disconnected.')
  else
    tLog.debug('Disconnecting...')
    self.fIsConnected = false
  end
end


function Plugin:detect()
  local tLog = self.tLog
  local tP = self.tPapaSchlumpf
  local ulPciBaseAddress = self.ulPciBaseAddress
  local tResult

  -- Read the start of the area. There should be the mailbox info block.
  local strDPM = tP:memReadArea(self.ulPciBaseAddress, self.sizMailboxInfo)
  -- Parse the info block.
  local tInfoBlock = self.tStructureMailboxInfo:read(strDPM)
  self.pl.pretty.dump(tInfoBlock)

  -- Check the magic.
  if tInfoBlock.aucMagic~=self.strMailboxMagic then
    tLog.error('No magic found.')
  else
    if tInfoBlock.ulVersion~=0x00010000 then
      tLog.error('Unexpected version found: 0x%08x', tInfoBlock.ulVersion)
    else
      -- Found a valid mailbox info block.
      tLog.info('Found mailbox.')

      local ulChipTyp = tInfoBlock.ulChipTyp
      local strChipId = self.atIdToRomloaderChipTyp[ulChipTyp]
      if strChipId==nil then
        tLog.error('Unknown chip type found: 0x%08x', ulChipTyp)
      else
        -- Get the control offsets.
        self.ulControlRxAddress = tP:MEMADR(ulPciBaseAddress, tInfoBlock.ulControlRxOffset)
        self.ulControlTxAddress = tP:MEMADR(ulPciBaseAddress, tInfoBlock.ulControlTxOffset)
        -- Get the buffer offsets.
        self.ulBufferRxAddress = tP:MEMADR(ulPciBaseAddress, tInfoBlock.ulBufferRxOffset)
        self.ulBufferTxAddress = tP:MEMADR(ulPciBaseAddress, tInfoBlock.ulBufferTxOffset)
        -- Get the buffer sizes.
        self.ulBufferRxSize = tInfoBlock.ulBufferRxSize
        self.ulBufferTxSize = tInfoBlock.ulBufferTxSize
        -- Set the chip type.
        self.ulChipTyp = ulChipTyp

        -- FIXME: this should go to a "connect" method.
        self.fIsConnected = true

        tResult = true
      end
    end
  end

  return tResult
end


function Plugin:sleep(fDelayInSeconds)
  os.execute(string.format('sleep %f', fDelayInSeconds))
end


function Plugin:sendMailboxData(strData)
  local tLog = self.tLog
  local tP = self.tPapaSchlumpf
  local ulControlRxAddress = self.ulControlRxAddress
  local sizMailboxControl = self.sizMailboxControl
  local tStructureMailboxControl = self.tStructureMailboxControl
  local bit = require 'bit'

  sizData = string.len(strData)
  -- Cowardly refuse to send 0 bytes.
  if sizData==0 then
    error('Refuse to send 0 bytes of data.')

  -- Does the data fit into the buffer?
  elseif sizData>self.ulBufferRxSize then
    error('The send data exceeds the buffer size.')
  end

  -- Wait until the mailbox is free.
  repeat
    local fMailboxIsFree
    -- Read the RX control block.
    local strControlBlock = tP:memReadArea(ulControlRxAddress, sizMailboxControl)
    local tControl = tStructureMailboxControl:read(strControlBlock)
    if tControl.ulReqCnt~=tControl.ulAckCnt then
      tLog.debug('The RX mailbox is full, retry in 100ms.')
      self:sleep(0.1)
    else
      fMailboxIsFree = true
    end
  until fMailboxIsFree==true

  -- Pad the data to the next DWORD.
  local sizPadded = bit.band(sizData+3, 0xfffffffc) - sizData
  local strDataPadded = strData .. string.rep(string.char(0), sizPadded)

--  tLog.debug('Sending packet with data:')
--  _G.tester:hexdump(strDataPadded)

  -- Write the data to the buffer.
  tP:memWriteArea(self.ulBufferRxAddress, strDataPadded)

  -- Set the size of the data.
  tP:memWrite(ulControlRxAddress+8, sizData)

  -- Increase the request counter.
  local ulReqCnt = tP:memRead(ulControlRxAddress)
  -- Increment and wrap the counter.
  ulReqCnt = bit.band(ulReqCnt+1, 0xffffffff)
  -- Write the new request counter to the mailbox.
  tP:memWrite(ulControlRxAddress, ulReqCnt)
end


function Plugin:receiveMailboxData()
  local bit = require 'bit'
  local tLog = self.tLog
  local tP = self.tPapaSchlumpf
  local ulControlTxAddress = self.ulControlTxAddress
  local tStructureMailboxControl = self.tStructureMailboxControl

  -- Wait for a response.
  local strResponse
  repeat
    -- Read the control block.
    local strControlBlock = tP:memReadArea(ulControlTxAddress, self.sizMailboxControl)
    local tControl = tStructureMailboxControl:read(strControlBlock)
    if tControl.ulReqCnt~=tControl.ulAckCnt then
      -- The mailbox is full.

      -- Test the data size.
      local ulSize = tControl.ulDataSize
      if ulSize==0 then
        error('Received a 0 byte packet.')
      elseif ulSize>self.ulBufferTxSize then
        error('The received packet claims to have more data than the buffer can hold.')
      end

      -- Round up the read size to the next DWORD.
      local strDataSizePadded = bit.band(ulSize+3, 0xfffffffc)
      local strResponsePadded = tP:memReadArea(self.ulBufferTxAddress, strDataSizePadded)
      strResponse = string.sub(strResponsePadded, 1, ulSize)

      -- Acknowledge the data.
      tP:memWrite(ulControlTxAddress+4, tControl.ulReqCnt)
    else
      -- Delay a little while.
      self:sleep(0.001)
    end
  until strResponse~=nil

  return strResponse
end


function Plugin:sendPacket(strData)
  local tLog = self.tLog
  local bit = require 'bit'
  local mhash = require 'mhash'

  -- Get the size of the data.
  local sizData = string.len(strData)
  -- Build a string representation of the size.
  local strSize = string.char(bit.band(sizData, 0xff), bit.band(bit.rshift(sizData,8), 0xff))
  -- Build the CRC for the size and data fields.
  local mh = mhash.mhash_state()
  mh:init(mhash.MHASH_CRC16)
  mh:hash(strSize)
  mh:hash(strData)
  local strCrc = mh:hash_end()

  -- Construct a packet from all components.
  local strPacket = self.strPacketStart .. strSize .. strData .. strCrc
  self:sendMailboxData(strPacket)
end


function Plugin:receivePacket()
  local tLog = self.tLog
  local mhash = require 'mhash'

  local strPacketData

  -- TODO: Use a persistent ringbuffer instead.
  local strBuffer = self:receiveMailboxData()
--  tLog.debug('Received packet with data:')
--  _G.tester:hexdump(strBuffer)

  -- Search the packet start.
  local iStart = string.find(strBuffer, self.strPacketStart, 1, true)
  if iStart==nil then
    tLog.debug('No packet start found.')
  else
    -- Keep only the data after the packet start.
    strBuffer = string.sub(strBuffer, iStart+1)

    -- Get the packet size.
    local usPacketSize = string.byte(strBuffer, 1) + 256 * string.byte(strBuffer, 2)
    -- Is enough data in the buffer for the packet?
    -- This includes usPacketSize bytes and 2 bytes of CRC.
    if string.len(strBuffer)<usPacketSize+2 then
      tLog.debug('Not enough data in the buffer')
    else
      local strData = string.sub(strBuffer, 3, usPacketSize+2)
      -- Get the CRC for the data.
      local mh = mhash.mhash_state()
      mh:init(mhash.MHASH_CRC16)
      mh:hash(string.sub(strBuffer, 1, 2))
      mh:hash(strData)
      local strCrcMy = mh:hash_end()
      -- Compare it with the CRC from the packet.
      local strCrcPacket = string.sub(strBuffer, usPacketSize+3, usPacketSize+4)
      if strCrcMy~=strCrcPacket then
        tLog.debug(
          'The packet CRC is invalid. My: 0x%04x, packet: 0x%04x.',
          string.byte(strCrcMy, 1) + 256 * string.byte(strCrcMy, 2),
          string.byte(strCrcPacket, 1) + 256 * string.byte(strCrcPacket, 2)
        )
      else
        -- The CRC matches, return the data.
        strPacketData = strData
      end
    end
  end

  return strPacketData
end


function Plugin:read_data08(ulAddress)
  local tLog = self.tLog
  local ucData

  -- Is the plugin connected?
  if self.fIsConnected~=true then
    error('Not connected.')
  end

  -- This is a read08 command at address 0. It should return 0x01 on a netX500.
  local strData = self.tStructurePacketReadWrite:write{
    ucType = self.MONITOR_PACKET_TYP.Command_Read08,
    ulAddress = ulAddress
  }
  self:sendPacket(strData)

  local strResponse = self:receivePacket()
  if strResponse==nil then
    error('Error')
  else
--    tLog.info('Got response:')
--    _G.tester:hexdump(strResponse)
    local ucType = string.byte(strResponse, 1)
    -- Is this a "read_data" packet?
    if ucType~=self.MONITOR_PACKET_TYP.Read_Data then
      _G.tester:hexdump(strResponse)
      error(string.format('Unexpected packet type: 0x%02x', ucType))
    else
      ucData = string.byte(strResponse, 2)
    end
  end

  return ucData
end


function Plugin:read_data16(ulAddress)
  local tLog = self.tLog
  local usData

  -- Is the plugin connected?
  if self.fIsConnected~=true then
    error('Not connected.')
  end

  -- This is a read16 command at address 0. It should return 0x0001 on a netX500.
  local strData = self.tStructurePacketReadWrite:write{
    ucType = self.MONITOR_PACKET_TYP.Command_Read16,
    ulAddress = ulAddress
  }
  self:sendPacket(strData)

  local strResponse = self:receivePacket()
  if strResponse==nil then
    error('Error')
  else
--    tLog.info('Got response:')
--    _G.tester:hexdump(strResponse)
    local ucType = string.byte(strResponse, 1)
    -- Is this a "read_data" packet?
    if ucType~=self.MONITOR_PACKET_TYP.Read_Data then
      _G.tester:hexdump(strResponse)
      error(string.format('Unexpected packet type: 0x%02x', ucType))
    else
      usData =              string.byte(strResponse, 2) +
               0x00000100 * string.byte(strResponse, 3)
    end
  end

  return usData
end


function Plugin:read_data32(ulAddress)
  local tLog = self.tLog
  local ulData

  -- Is the plugin connected?
  if self.fIsConnected~=true then
    error('Not connected.')
  end

  -- This is a read32 command at address 0. It should return 0xea080001 on a netX500.
  local strData = self.tStructurePacketReadWrite:write{
    ucType = self.MONITOR_PACKET_TYP.Command_Read32,
    ulAddress = ulAddress
  }
  self:sendPacket(strData)

  local strResponse = self:receivePacket()
  if strResponse==nil then
    error('Error')
  else
--    tLog.info('Got response:')
--    _G.tester:hexdump(strResponse)
    local ucType = string.byte(strResponse, 1)
    -- Is this a "read_data" packet?
    if ucType~=self.MONITOR_PACKET_TYP.Read_Data then
      _G.tester:hexdump(strResponse)
      error(string.format('Unexpected packet type: 0x%02x', ucType))
    else
      ulData =              string.byte(strResponse, 2) +
               0x00000100 * string.byte(strResponse, 3) +
               0x00010000 * string.byte(strResponse, 4) +
               0x01000000 * string.byte(strResponse, 5)
    end
  end

  return ulData
end


function Plugin:read_data64(ulAddress)
  local tLog = self.tLog
  local ullData

  -- Is the plugin connected?
  if self.fIsConnected~=true then
    error('Not connected.')
  end

  local strData = self.tStructurePacketReadWrite:write{
    ucType = self.MONITOR_PACKET_TYP.Command_Read64,
    ulAddress = ulAddress
  }
  self:sendPacket(strData)

  local strResponse = self:receivePacket()
  if strResponse==nil then
    error('Error')
  else
--    tLog.info('Got response:')
--    _G.tester:hexdump(strResponse)
    local ucType = string.byte(strResponse, 1)
    -- Is this a "read_data" packet?
    if ucType~=self.MONITOR_PACKET_TYP.Read_Data then
      _G.tester:hexdump(strResponse)
      error(string.format('Unexpected packet type: 0x%02x', ucType))
    else
      ulData =                      string.byte(strResponse, 2) +
               0x0000000000000100 * string.byte(strResponse, 3) +
               0x0000000000010000 * string.byte(strResponse, 4) +
               0x0000000001000000 * string.byte(strResponse, 5) +
               0x0000000100000000 * string.byte(strResponse, 6) +
               0x0000010000000000 * string.byte(strResponse, 7) +
               0x0001000000000000 * string.byte(strResponse, 8) +
               0x0100000000000000 * string.byte(strResponse, 9)
    end
  end

  return ulData
end


function Plugin:read_image(ulAddress, ulSize, fnCallback, pvCallback)
  local tLog = self.tLog
  local tData = {}

  -- Is the plugin connected?
  if self.fIsConnected~=true then
    error('Not connected.')
  end

  -- Get the maximum number of bytes in a "read_data" packet.
  local sizMaxChunk = self.ulBufferTxSize - 6

  -- Loop over the complete request in chunks.
  local ulOffset = 0
  repeat
    -- Get the next chunk.
    local ulChunk = ulSize - ulOffset
    if ulChunk>sizMaxChunk then
      ulChunk = sizMaxChunk
    end

    -- Read the data.
    local strData = self.tStructurePacketReadArea:write{
      ucType = self.MONITOR_PACKET_TYP.Command_ReadArea,
      ulAddress = ulAddress + ulOffset,
      usSize = ulChunk
    }
    self:sendPacket(strData)

    local strResponse = self:receivePacket()
    if strResponse==nil then
      error('Error')
    end
    local ucType = string.byte(strResponse, 1)
    -- Is this a "read_data" packet?
    if ucType~=self.MONITOR_PACKET_TYP.Read_Data then
      error(string.format('Unexpected packet type: 0x%02x', ucType))
    end

    -- Add the data to the output table.
    table.insert(tData, string.sub(strResponse, 2))

    tLog.info('0x%08x 0x%08x 0x%08x', ulOffset, ulSize, ulChunk)
    ulOffset = ulOffset + ulChunk

    -- Enter the callback function.
    local fContinue = true
    if fnCallback~=nil then
      fContinue = fnCallback(ulOffset, pvCallback)
    end
    if fContinue~=true then
      error('User cancel!')
    end
  until ulOffset>=ulSize

  return table.concat(tData)
end


function Plugin:write_data08(ulAddress, ucData)
  local tLog = self.tLog

  -- Is the plugin connected?
  if self.fIsConnected~=true then
    error('Not connected.')
  end

  local strData = self.tStructurePacketReadWrite:write{
    ucType = self.MONITOR_PACKET_TYP.Command_Write08,
    ulAddress = ulAddress
  } .. string.char(ucData)
  self:sendPacket(strData)

  local strResponse = self:receivePacket()
  if strResponse==nil then
    error('Error')
  else
--    tLog.info('Got response:')
--    _G.tester:hexdump(strResponse)
    local ucType = string.byte(strResponse, 1)
    -- Is this a "status" packet?
    if ucType~=self.MONITOR_PACKET_TYP.Status then
      error(string.format('Unexpected packet type: 0x%02x', ucType))
    else
      local ucStatus = string.byte(strResponse, 2)
      if ucStatus~=self.MONITOR_STATUS.Ok then
        error('Status is not OK')
      end
    end
  end
end


function Plugin:write_data16(ulAddress, usData)
  local bit = require 'bit'
  local tLog = self.tLog

  -- Is the plugin connected?
  if self.fIsConnected~=true then
    error('Not connected.')
  end

  local strData = self.tStructurePacketReadWrite:write{
    ucType = self.MONITOR_PACKET_TYP.Command_Write16,
    ulAddress = ulAddress
  } .. string.char(
    bit.band(           usData,      0xff),
    bit.band(bit.rshift(usData,  8), 0xff)
  )
  self:sendPacket(strData)

  local strResponse = self:receivePacket()
  if strResponse==nil then
    error('Error')
  else
--    tLog.info('Got response:')
--    _G.tester:hexdump(strResponse)
    local ucType = string.byte(strResponse, 1)
    -- Is this a "status" packet?
    if ucType~=self.MONITOR_PACKET_TYP.Status then
      error(string.format('Unexpected packet type: 0x%02x', ucType))
    else
      local ucStatus = string.byte(strResponse, 2)
      if ucStatus~=self.MONITOR_STATUS.Ok then
        error('Status is not OK')
      end
    end
  end
end


function Plugin:write_data32(ulAddress, ulData)
  local bit = require 'bit'
  local tLog = self.tLog

  -- Is the plugin connected?
  if self.fIsConnected~=true then
    error('Not connected.')
  end

  local strData = self.tStructurePacketReadWrite:write{
    ucType = self.MONITOR_PACKET_TYP.Command_Write32,
    ulAddress = ulAddress
  } .. string.char(
    bit.band(           ulData,      0xff),
    bit.band(bit.rshift(ulData,  8), 0xff),
    bit.band(bit.rshift(ulData, 16), 0xff),
    bit.band(bit.rshift(ulData, 24), 0xff)
  )
  self:sendPacket(strData)

  local strResponse = self:receivePacket()
  if strResponse==nil then
    error('Error')
  else
--    tLog.info('Got response:')
--    _G.tester:hexdump(strResponse)
    local ucType = string.byte(strResponse, 1)
    -- Is this a "status" packet?
    if ucType~=self.MONITOR_PACKET_TYP.Status then
      error(string.format('Unexpected packet type: 0x%02x', ucType))
    else
      local ucStatus = string.byte(strResponse, 2)
      if ucStatus~=self.MONITOR_STATUS.Ok then
        error('Status is not OK')
      end
    end
  end
end


function Plugin:write_data64(ulAddress, ullData)
  local bit = require 'bit'
  local tLog = self.tLog

  -- Is the plugin connected?
  if self.fIsConnected~=true then
    error('Not connected.')
  end

  local strData = self.tStructurePacketReadWrite:write{
    ucType = self.MONITOR_PACKET_TYP.Command_Write64,
    ulAddress = ulAddress
  } .. string.char(
    bit.band(           ullData,      0xff),
    bit.band(bit.rshift(ullData,  8), 0xff),
    bit.band(bit.rshift(ullData, 16), 0xff),
    bit.band(bit.rshift(ullData, 24), 0xff),
    bit.band(bit.rshift(ullData, 32), 0xff),
    bit.band(bit.rshift(ullData, 40), 0xff),
    bit.band(bit.rshift(ullData, 48), 0xff),
    bit.band(bit.rshift(ullData, 56), 0xff)
  )
  self:sendPacket(strData)

  local strResponse = self:receivePacket()
  if strResponse==nil then
    error('Error')
  else
--    tLog.info('Got response:')
--    _G.tester:hexdump(strResponse)
    local ucType = string.byte(strResponse, 1)
    -- Is this a "status" packet?
    if ucType~=self.MONITOR_PACKET_TYP.Status then
      error(string.format('Unexpected packet type: 0x%02x', ucType))
    else
      local ucStatus = string.byte(strResponse, 2)
      if ucStatus~=self.MONITOR_STATUS.Ok then
        error('Status is not OK')
      end
    end
  end
end


function Plugin:write_image(ulAddress, strData, fnCallback, pvCallback)
  local tLog = self.tLog

  -- Is the plugin connected?
  if self.fIsConnected~=true then
    error('Not connected.')
  end

  -- Get the maximum number of bytes in a "write_area" packet.
  -- This is the maximum packet size minus...
  --   1 byte stream start
  --   2 bytes data size
  --   1 byte packet type
  --   4 bytes address
  --   2 bytes CRC
  -- which is a total of 10 bytes.
  local sizMaxChunk = self.ulBufferTxSize - 10

  -- Loop over the complete request in chunks.
  local ulSize = string.len(strData)
  local ulOffset = 0
  repeat
    -- Get the next chunk.
    local ulChunk = ulSize - ulOffset
    if ulChunk>sizMaxChunk then
      ulChunk = sizMaxChunk
    end

    local strData = self.tStructurePacketReadWrite:write{
      ucType = self.MONITOR_PACKET_TYP.Command_WriteArea,
      ulAddress = ulAddress + ulOffset
    } .. string.sub(strData, ulOffset+1, ulOffset+ulChunk)
    self:sendPacket(strData)

    local strResponse = self:receivePacket()
    if strResponse==nil then
      error('Error')
    else
--      tLog.info('Got response:')
--      _G.tester:hexdump(strResponse)
      local ucType = string.byte(strResponse, 1)
      -- Is this a "status" packet?
      if ucType~=self.MONITOR_PACKET_TYP.Status then
        error(string.format('Unexpected packet type: 0x%02x', ucType))
      else
        local ucStatus = string.byte(strResponse, 2)
        if ucStatus~=self.MONITOR_STATUS.Ok then
          error('Status is not OK')
        end
      end
    end

    ulOffset = ulOffset + ulChunk

    -- Enter the callback function.
    local fContinue = true
    if fnCallback~=nil then
      fContinue = fnCallback(ulOffset, pvCallback)
    end
    if fContinue~=true then
      error('User cancel!')
    end
  until ulOffset>=ulSize
end


function Plugin:call(ulAddress, ulParameterR0, fnCallback, pvCallback)
  local tLog = self.tLog

  -- Is the plugin connected?
  if self.fIsConnected~=true then
    error('Not connected.')
  end

  local strData = self.tStructurePacketCall:write{
    ucType = self.MONITOR_PACKET_TYP.Command_Call,
    ulAddress = ulAddress,
    ulR0 = ulParameterR0
  }
  self:sendPacket(strData)

  repeat
    local fCallFinished = false
    local strResponse = self:receivePacket()
    if strResponse==nil then
      error('Error')
    else
      tLog.info('Got response:')
      _G.tester:hexdump(strResponse)
      local ucType = string.byte(strResponse, 1)
      if ucType==self.MONITOR_PACKET_TYP.Status then
        local ucStatus = string.byte(strResponse, 2)
        if ucStatus==self.MONITOR_STATUS.Call_Finished then
          fCallFinished = true
        end
      elseif ucType==self.MONITOR_PACKET_TYP.Call_Data then
        if fnCallback~=nil then
          fnCallback(string.sub(strResponse, 2), pvCallback)
        end
      end
    end
  until fCallFinished==true
end


function Plugin:test()
  local tLog = self.tLog

  tLog.info('Starting communication test.')
  local tResult = self:detect()
  if tResult==true then
    -- Send some demo packets.
    local ucData = self:read_data08(0x00200000)
    tLog.info('Read: 0x%02x', ucData)
    local usData = self:read_data16(0x00200000)
    tLog.info('Read: 0x%04x', usData)
    local ulData = self:read_data32(0x00200000)
    tLog.info('Read: 0x%08x', ulData)
    local ullData = self:read_data64(0x00200000)
    tLog.info('Read: 0x%016x', ullData)

    -- Write data to the test area.
    local ulTestArea = 0x8000
    self:write_data08(ulTestArea+0x00, 0x01)
    self:write_data08(ulTestArea+0x01, 0x23)
    self:write_data08(ulTestArea+0x02, 0x45)
    self:write_data08(ulTestArea+0x03, 0x67)
    self:write_data16(ulTestArea+0x04, 0x89ab)
    self:write_data16(ulTestArea+0x06, 0xcdef)
    self:write_data32(ulTestArea+0x08, 0x76543210)
    self:write_data32(ulTestArea+0x0c, 0xfedcba98)

    -- Rad back the test area.
    local strReadback = self:read_image(ulTestArea, 0x10)
    _G.tester:hexdump(strReadback)

    -- Read a bit of the ROM area.
    local strRom = self:read_image(0x00200000, 0x00008000)
--    self.pl.utils.writefile('netx500.rom', strRom, true)

    -- Write the ROM to the test area.
    self:write_image(ulTestArea, strRom)
    -- Read back.
    strReadback = self:read_image(ulTestArea, 0x8000)
    if strRom~=strReadback then
      self.pl.utils.writefile('netx500.rom', strRom, true)
      self.pl.utils.writefile('readback.bin', strReadback, true)
      error('Failed!')
    end
  end

  tLog.info('Communication test finished.')
end


return Plugin
