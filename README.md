Vivado_projects
===============

A repository for some of my Vivado projects targeting Xilinx FPGAs

my_first_microblaze
===============

A simple microblaze system.

HOW TO:
* source /xx/yy/Vivado/version/setting64.sh
* make

TODO:
* The generated project includes twice the xdc constraint file. I could not understand where the problem is. It is not a huge bug, but it would be nice to remove it. 

NOTES:
===============
The .tcl files setting up the project (my_first_microblaze.tcl) and the one creating the bd (bd.tcl) can be created automatically from a project opened in Vivado. This is explained in ug994, Chapter 9.

The command write_bd_tcl is used to create the bd.tcl file.

The command write_project_tcl is used to create the my_first_microblaze.tcl file. However the tcl file is created to incude directly a bd file, not to use the bd.tcl file. So we remove the lines including the bd file and we add the following:

# Create BD
source bd.tcl
validate_bd_design
save_bd_design
#Create top wrapper file
make_wrapper -files [get_files $proj_dir/$proj_name.srcs/sources_1/bd/microblaze_system_1/microblaze_system_1.bd] -top
import_files -force -norecurse $proj_dir/$proj_name.srcs/sources_1/bd/microblaze_system_1/hdl/microblaze_system_1_wrapper.vhd
