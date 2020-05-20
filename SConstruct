# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------#
#   Copyright (C) 2018 by Christoph Thelen                                #
#   doc_bacardi@users.sourceforge.net                                     #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
#-------------------------------------------------------------------------#


#----------------------------------------------------------------------------
#
# Set up the Muhkuh Build System.
#
SConscript('mbs/SConscript')
Import('atEnv')

# Create a build environment for the ARM9 based netX chips.
env_arm9 = atEnv.DEFAULT.CreateEnvironment(['gcc-arm-none-eabi-9.2', 'asciidoc'])
env_arm9.CreateCompilerEnv('NETX500', ['arm', 'arch=armv5te+fp', 'float-abi=softfp'])

# Build the platform libraries.
SConscript('platform/SConscript')


#----------------------------------------------------------------------------
#
# Get the source code version from the VCS.
#
atEnv.DEFAULT.Version('#targets/version/version.h', 'templates/version.h')


#----------------------------------------------------------------------------
# This is the list of sources. The elements must be separated with whitespace
# (i.e. spaces, tabs, newlines). The amount of whitespace does not matter.
sources = """
    src/header.c
    src/init.S
    src/main.c
    src/pci.c
    src/uprintf_buffer.c
    src/usb_emsys/usb.c
    src/usb_emsys/usb_descriptors.c
    src/usb_emsys/usb_globals.c
    src/usb_emsys/usb_io.c
    src/usb_emsys/usb_main.c
    src/usb_emsys/usb_requests_top.c
    src/usb_emsys/usb_command_execution.c
"""

#----------------------------------------------------------------------------
#
# Build all files.
#

# The list of include folders. Here it is used for all files.
astrIncludePaths = ['src', '#platform/src', '#platform/src/lib', '#targets/version']

tEnv = atEnv.NETX500.Clone()
tEnv.Append(CPPPATH = astrIncludePaths)
tEnv.Replace(LDFILE = 'src/netx500.ld')
tSrc = tEnv.SetBuildPath('targets/netx500_intram', 'src', sources)
tElf = tEnv.Elf('targets/netx500_intram/netx500_intram.elf', tSrc + tEnv['PLATFORM_LIBRARY'])
tImg = tEnv.BootBlock('targets/papa_schlumpf_flex.img', tElf, BOOTBLOCK_SRC='SPI_GEN_10', BOOTBLOCK_DST='INTRAM')
