Vivado_projects
===============

A repository for some of my Vivado projects targeting Xilinx FPGAs

AMP_baremetal_zynq
===============

Takig inspiration from http://www.wiki.xilinx.com/XAPP1079+Latest+Information and making this example working with a more "open" approach (e.g. no vio - ila - debug cores requiring licenses)
Now it works so that we can use buttont (NSWE) to generate interrupts on both edges (push down, release), while the central button is generating interrupts only when it is released.

HOWTO:
follow instructions in docs/Vivado instructions.txt
vivado -mode tcl source ...tcl

NOTE:
Serial config: 115000 8N1

TODO: study the PBL and linker files

BSP notes
===============

1]
  * folder AMP_baremetal_zynq/design/src/sdk_repo/bsp/standalone_v4_91/src/cortexa9/gcc in THIS PROJECT
  IS EQUIVALENT TO
  * folder <prj_name>/<prj_name>.sdk/<sw_prj_name>_bsp/ps7_cortexa9_0/libsrc/standalone_v4_2/src
  * These two folder contains the boot.S file, which is very similar but not completely...
  * The only "valuable" files differing are boot.S and asm_vectors.S

2]
  * the files present in folder <prj_name>/<prj_name>.sdk/<sw_prj_name>_bsp/ps7_cortexa9_0/libsrc/standalone_v4_2/src
  which are missing in AMP_baremetal_zynq/design/src/sdk_repo/bsp/standalone_v4_91/src/cortexa9/gcc are present in
  AMP_baremetal_zynq/design/src/sdk_repo/bsp/standalone_v4_91/src/cortexa9/ ... looks like some file directories have been mixed

2]
  * note that the folders contain a xil-crt0.s