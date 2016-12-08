##############################################################################
## Filename:          C:\Users\superman\Desktop\SpaceInvaders\SpaceInvaders/drivers/dmacontroller_v1_00_a/data/dmacontroller_v2_1_0.tcl
## Description:       Microprocess Driver Command (tcl)
## Date:              Thu Dec 08 10:41:09 2016 (by Create and Import Peripheral Wizard)
##############################################################################

#uses "xillib.tcl"

proc generate {drv_handle} {
  xdefine_include_file $drv_handle "xparameters.h" "dmacontroller" "NUM_INSTANCES" "DEVICE_ID" "C_BASEADDR" "C_HIGHADDR" 
}
