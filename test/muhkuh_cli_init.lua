-- Set the search path for LUA plugins and modules.
package.cpath = package.cpath .. ";lua_plugins/?.so"
package.path = package.path .. ";lua/?.lua;lua/?/init.lua"

-- Load all plugins.
-- require 'romloader_eth'
-- require 'romloader_jtag'
-- require 'romloader_papa_schlumpf'
-- require 'romloader_uart'
-- require 'romloader_usb'
