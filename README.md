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