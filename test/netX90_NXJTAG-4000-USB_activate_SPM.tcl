proc read_data32 {addr} {
  set value(0) 0
  mem2array value 32 $addr 1
  return $value(0)
}

proc probe {} {
  global SC_CFG_RESULT
  set SC_CFG_RESULT 0
  set RESULT -1

  # Disable all servers.
  gdb_port disabled
  tcl_port disabled
  telnet_port disabled

  # Setup the interface.
  interface ftdi
  transport select jtag
  ftdi_device_desc "NXJTAG-4000-USB"
  ftdi_vid_pid 0x1939 0x0301
  adapter_khz 1000
  ftdi_layout_init 0x1B08 0x1F0B
  ftdi_layout_signal nTRST -data 0x0100 -oe 0x0100
  ftdi_layout_signal nSRST -data 0x0200 -oe 0x0200
  ftdi_layout_signal JSEL1 -data 0x0400 -oe 0x0400
  ftdi_layout_signal VODIS -data 0x0800 -oe 0x0800
  ftdi_layout_signal VOSWI -data 0x1000 -oe 0x1000

  # Expect a netX90 scan chain.
  jtag newtap netx90 dap -expected-id 0x6ba00477 -irlen 4 -enable
  jtag newtap netx90 tap -expected-id 0x10a046ad -irlen 4 -enable
  jtag configure netx90.dap -event setup { global SC_CFG_RESULT ; echo {Yay - setup netx 90} ; set SC_CFG_RESULT {OK} }

  # Expect working SRST and TRST lines.
  reset_config trst_and_srst

  # Try to initialize the JTAG layer.
  if {[ catch {jtag init} ]==0 } {
    if { $SC_CFG_RESULT=={OK} } {
      target create netx90.comm cortex_m -chain-position netx90.dap -coreid 0 -ap-num 2
      netx90.comm configure -event reset-init { halt }
      cortex_m reset_config srst

      init

      # Try to stop the CPU.
      halt

      cortex_m vector_catch none

      # Load the INTRAM image which activates SPM booting.
      load_image SRC_DPM0xSRC_SER.img 0x20080000 bin
      load_image SRC_DPM0xSRC_SER.img 0x20080000 bin

      sleep 500

      # Reset.
      mww 0xff4012c0 [read_data32 0xff4012c0]
      mww 0xff0016b0 0x01000000

      # The board should blink yellow now long-short-short .
      sleep 500

      set RESULT 0
    }
  }

  return $RESULT
}

probe
