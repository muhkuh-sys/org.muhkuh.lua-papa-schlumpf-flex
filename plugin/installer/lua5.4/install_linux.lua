local t = ...
local strDistId, strDistVersion, strCpuArch = t:get_platform()
local tResult

if strDistId=='@JONCHKI_PLATFORM_DIST_ID@' and strDistVersion=='@JONCHKI_PLATFORM_DIST_VERSION@' and strCpuArch=='@JONCHKI_PLATFORM_CPU_ARCH@' then
  t:install('lua_plugins/', '${install_lua_cpath}/')
  t:install('lua/', '${install_lua_path}/')
  t:install('netx/', '${install_base}/netx/')
  tResult = true
end

return tResult
