

# Create project
set proj_name project_1
create_project -force $proj_name ./$proj_name

# Set project properties
set obj [get_projects $proj_name]
set_property "part" "xc7z020clg484-1" $obj
set_property "target_language" "Verilog" $obj
set_property board_part em.avnet.com:zed:part0:1.0 $obj

# Set the directory path for the new project
set proj_dir [get_property directory $obj]

set_property ip_repo_paths  $proj_dir/../../pcores [current_fileset]
update_ip_catalog

#create BD
source $proj_dir/../../scripts/create_bd_zedBoard.tcl
validate_bd_design
save_bd_design

#Create top wrapper file
make_wrapper -files [get_files $proj_dir/$proj_name.srcs/sources_1/bd/design_1/design_1.bd] -top
import_files -force -norecurse $proj_dir/$proj_name.srcs/sources_1/bd/design_1/hdl/design_1_wrapper.v

#implement the design and create bit file
launch_runs impl_1 -to_step write_bitstream
wait_on_run -timeout 60 impl_1

#Open implementation in order to export design to SDK
open_run impl_1

#Export design to SDK
export_hardware [get_files $proj_dir/$proj_name.srcs/sources_1/bd/design_1/design_1.bd] [get_runs impl_1] -bitstream
launch_sdk -bit $proj_dir/$proj_name.sdk/SDK/SDK_Export/hw/design_1_wrapper.bit -workspace $proj_dir/$proj_name.sdk/SDK/SDK_Export -hwspec $proj_dir/$proj_name.sdk/SDK/SDK_Export/hw/design_1.xml


