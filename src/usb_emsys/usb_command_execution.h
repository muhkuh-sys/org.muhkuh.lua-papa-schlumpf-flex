/*
 * command_execution.h
 *
 *  Created on: 18.05.2016
 *      Author: DennisKaiser
 */

#include "usb.h"
#include "../common/papa_schlumpf_firmware_interface.h"


#ifndef NETX_SRC_COMMAND_EXECUTION_H_
#define NETX_SRC_COMMAND_EXECUTION_H_


void execute_command(PAPA_SCHLUMPF_USB_COMMAND_T *ptCommand);

#endif /* NETX_SRC_COMMAND_EXECUTION_H_ */
