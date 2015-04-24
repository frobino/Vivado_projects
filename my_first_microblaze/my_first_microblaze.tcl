# Set the reference directory for source file relative paths (by default the value is script directory path)
set origin_dir "."

# Set the directory path for the original project from where this script was exported
set orig_proj_dir "[file normalize "$origin_dir/my_first_microblaze"]"

# Set project name
set prj_name "my_first_microblaze"

# Create project
create_project $prj_name ./my_first_microblaze

# Set the directory path for the new project
set proj_dir [get_property directory [current_project]]

# Set project properties
set obj [get_projects $prj_name]
set_property "board_part" "em.avnet.com:zed:part0:1.2" $obj
set_property "default_lib" "xil_defaultlib" $obj
set_property "simulator_language" "Mixed" $obj
set_property "target_language" "VHDL" $obj

# Create 'sources_1' fileset (if not found)
if {[string equal [get_filesets -quiet sources_1] ""]} {
  create_fileset -srcset sources_1
}






###########################################################################################
## Add the following part to try instantiate ublaze
###########################################################################################

# Hierarchical cell: microblaze_1_local_memory
proc create_hier_cell_microblaze_1_local_memory { parentCell nameHier } {

  if { $parentCell eq "" || $nameHier eq "" } {
     puts "ERROR: create_hier_cell_microblaze_1_local_memory() - Empty argument(s)!"
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     puts "ERROR: Unable to find parent cell <$parentCell>!"
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     puts "ERROR: Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode MirroredMaster -vlnv xilinx.com:interface:lmb_rtl:1.0 DLMB
  create_bd_intf_pin -mode MirroredMaster -vlnv xilinx.com:interface:lmb_rtl:1.0 ILMB

  # Create pins
  create_bd_pin -dir I LMB_Clk
  create_bd_pin -dir I -from 0 -to 0 LMB_Rst

  # Create instance: dlmb_v10, and set properties
  set dlmb_v10 [ create_bd_cell -type ip -vlnv xilinx.com:ip:lmb_v10:3.0 dlmb_v10 ]

  # Create instance: ilmb_v10, and set properties
  set ilmb_v10 [ create_bd_cell -type ip -vlnv xilinx.com:ip:lmb_v10:3.0 ilmb_v10 ]

  # Create instance: dlmb_bram_if_cntlr, and set properties
  set dlmb_bram_if_cntlr [ create_bd_cell -type ip -vlnv xilinx.com:ip:lmb_bram_if_cntlr:4.0 dlmb_bram_if_cntlr ]
  set_property -dict [ list CONFIG.C_ECC {0}  ] $dlmb_bram_if_cntlr

  # Create instance: ilmb_bram_if_cntlr, and set properties
  set ilmb_bram_if_cntlr [ create_bd_cell -type ip -vlnv xilinx.com:ip:lmb_bram_if_cntlr:4.0 ilmb_bram_if_cntlr ]
  set_property -dict [ list CONFIG.C_ECC {0}  ] $ilmb_bram_if_cntlr

  # Create instance: lmb_bram, and set properties
  set lmb_bram [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.2 lmb_bram ]
  set_property -dict [ list CONFIG.Memory_Type {True_Dual_Port_RAM} CONFIG.use_bram_block {BRAM_Controller}  ] $lmb_bram

  # Create interface connections
  connect_bd_intf_net -intf_net microblaze_1_dlmb_bus [get_bd_intf_pins dlmb_v10/LMB_Sl_0] [get_bd_intf_pins dlmb_bram_if_cntlr/SLMB]
  connect_bd_intf_net -intf_net microblaze_1_ilmb_bus [get_bd_intf_pins ilmb_v10/LMB_Sl_0] [get_bd_intf_pins ilmb_bram_if_cntlr/SLMB]
  connect_bd_intf_net -intf_net microblaze_1_dlmb_cntlr [get_bd_intf_pins dlmb_bram_if_cntlr/BRAM_PORT] [get_bd_intf_pins lmb_bram/BRAM_PORTA]
  connect_bd_intf_net -intf_net microblaze_1_ilmb_cntlr [get_bd_intf_pins ilmb_bram_if_cntlr/BRAM_PORT] [get_bd_intf_pins lmb_bram/BRAM_PORTB]
  connect_bd_intf_net -intf_net microblaze_1_dlmb [get_bd_intf_pins DLMB] [get_bd_intf_pins dlmb_v10/LMB_M]
  connect_bd_intf_net -intf_net microblaze_1_ilmb [get_bd_intf_pins ILMB] [get_bd_intf_pins ilmb_v10/LMB_M]

  # Create port connections
  connect_bd_net -net microblaze_1_Clk [get_bd_pins LMB_Clk] [get_bd_pins dlmb_v10/LMB_Clk] [get_bd_pins dlmb_bram_if_cntlr/LMB_Clk] [get_bd_pins ilmb_v10/LMB_Clk] [get_bd_pins ilmb_bram_if_cntlr/LMB_Clk]
  connect_bd_net -net proc_sys_reset_1_bus_struct_reset [get_bd_pins LMB_Rst] [get_bd_pins dlmb_v10/SYS_Rst] [get_bd_pins dlmb_bram_if_cntlr/LMB_Rst] [get_bd_pins ilmb_v10/SYS_Rst] [get_bd_pins ilmb_bram_if_cntlr/LMB_Rst]
  
  # Restore current instance
  current_bd_instance $oldCurInst
}


# Procedure to create entire design; Provide argument to make
# procedure reusable. If parentCell is "", will use root.
proc create_root_design { parentCell } {

  if { $parentCell eq "" } {
     set parentCell [get_bd_cells /]
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     puts "ERROR: Unable to find parent cell <$parentCell>!"
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     puts "ERROR: Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."
     return
  }

  puts "HERE I AM 1."
     
  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  puts "HERE I AM 2."

  
  # Set parent object as current
  current_bd_instance $parentObj

  puts "HERE I AM 3."

  # Create instance: axi_bram_ctrl_1, and set properties
  set axi_bram_ctrl_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.0 axi_bram_ctrl_1 ]

  puts "HERE I AM 4."


  # Create instance: proc_sys_reset, and set properties
  set proc_sys_reset [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset ]

  puts "HERE I AM 5."

  # Create instance: blk_mem_gen_1, and set properties
  set blk_mem_gen_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:blk_mem_gen:8.2 blk_mem_gen_1 ]
  set_property -dict [ list CONFIG.Memory_Type {True_Dual_Port_RAM}  ] $blk_mem_gen_1

    puts "HERE I AM 6."

  # Create instance: microblaze_1, and set properties
  set microblaze_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:microblaze:9.4 microblaze_1 ]
  set_property -dict [ list CONFIG.C_FAULT_TOLERANT {0} CONFIG.C_BASE_VECTORS {0x00000000} CONFIG.C_D_AXI {1} CONFIG.C_D_LMB {1} CONFIG.C_I_AXI {1} CONFIG.C_I_LMB {1} CONFIG.C_USE_DIV {1} CONFIG.C_USE_HW_MUL {2} CONFIG.C_USE_FPU {2} CONFIG.C_DEBUG_ENABLED {1} CONFIG.C_USE_INTERRUPT {0}  ] $microblaze_1

    puts "HERE I AM 7."

  # Create instance: microblaze_1_local_memory
  create_hier_cell_microblaze_1_local_memory [current_bd_instance .] microblaze_1_local_memory

  # Create instance: mdm_1, and set properties
  set mdm_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:mdm:3.2 mdm_1 ]

  # Create instance: axi_mem_intercon, and set properties
  set axi_mem_intercon [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_mem_intercon ]
  set_property -dict [ list CONFIG.NUM_SI {2} CONFIG.NUM_MI {1}  ] $axi_mem_intercon

  # Create interface connections
  connect_bd_intf_net -intf_net axi_bram_ctrl_1_bram_porta [get_bd_intf_pins blk_mem_gen_1/BRAM_PORTA] [get_bd_intf_pins axi_bram_ctrl_1/BRAM_PORTA]
  connect_bd_intf_net -intf_net microblaze_1_dlmb [get_bd_intf_pins microblaze_1/DLMB] [get_bd_intf_pins microblaze_1_local_memory/DLMB]
  connect_bd_intf_net -intf_net microblaze_1_ilmb [get_bd_intf_pins microblaze_1/ILMB] [get_bd_intf_pins microblaze_1_local_memory/ILMB]
  connect_bd_intf_net -intf_net microblaze_1_debug [get_bd_intf_pins mdm_1/MBDEBUG_0] [get_bd_intf_pins microblaze_1/DEBUG]
  connect_bd_intf_net -intf_net axi_bram_ctrl_1_bram_portb [get_bd_intf_pins blk_mem_gen_1/BRAM_PORTB] [get_bd_intf_pins axi_bram_ctrl_1/BRAM_PORTB]
  connect_bd_intf_net -intf_net microblaze_1_m_axi_dp [get_bd_intf_pins microblaze_1/M_AXI_DP] [get_bd_intf_pins axi_mem_intercon/S00_AXI]
  connect_bd_intf_net -intf_net axi_mem_intercon_m00_axi [get_bd_intf_pins axi_mem_intercon/M00_AXI] [get_bd_intf_pins axi_bram_ctrl_1/S_AXI]
  set_property -dict [ list HDL_ATTRIBUTE.MARK_DEBUG {true}  ] [get_bd_intf_nets axi_mem_intercon_m00_axi]
  
  # Create port connections
  connect_bd_net -net proc_sys_reset_1_peripheral_aresetn [get_bd_pins axi_bram_ctrl_1/S_AXI_ARESETN] [get_bd_pins axi_mem_intercon/S00_ARESETN] [get_bd_pins axi_mem_intercon/M00_ARESETN] [get_bd_pins axi_mem_intercon/S01_ARESETN]
  connect_bd_net -net proc_sys_reset_1_bus_struct_reset [get_bd_pins proc_sys_reset/bus_struct_reset] [get_bd_pins microblaze_1_local_memory/LMB_Rst]
  connect_bd_net -net mdm_1_debug_sys_rst [get_bd_pins mdm_1/Debug_SYS_Rst] [get_bd_pins proc_sys_reset/mb_debug_sys_rst]
  connect_bd_net -net proc_sys_reset_1_mb_reset [get_bd_pins proc_sys_reset/mb_reset] [get_bd_pins microblaze_1/Reset]
  connect_bd_net -net proc_sys_reset_interconnect_aresetn [get_bd_pins proc_sys_reset/interconnect_aresetn] [get_bd_pins axi_mem_intercon/ARESETN]

  # Create address segments
  create_bd_addr_seg -range 0x1000 -offset 0x40000000 [get_bd_addr_spaces microblaze_1/Data] [get_bd_addr_segs axi_bram_ctrl_1/S_AXI/Mem0] SEG1
  create_bd_addr_seg -range 0x2000 -offset 0x0 [get_bd_addr_spaces microblaze_1/Data] [get_bd_addr_segs microblaze_1_local_memory/dlmb_bram_if_cntlr/SLMB/Mem] SEG2
  create_bd_addr_seg -range 0x2000 -offset 0x0 [get_bd_addr_spaces microblaze_1/Instruction] [get_bd_addr_segs microblaze_1_local_memory/ilmb_bram_if_cntlr/SLMB/Mem] SEG1
  

  # Restore current instance
  current_bd_instance $oldCurInst
}


## Add the following part to try instantiate ublaze
## 
## # Creating design if needed
## if { [get_files *.bd] eq "" } {
##    puts "INFO: Currently there are no designs in project, so creating one..."
##    create_bd_design design_1
## }
## 
## create_root_design ""

###########################################################################################







# Set 'sources_1' fileset object
set obj [get_filesets sources_1]
set files [list \
 "[file normalize "$origin_dir/bd_sources/microblaze_system_1.bd"]"\
 "[file normalize "$origin_dir/hdl_sources/microblaze_system_1_wrapper.vhd"]"\
]
add_files -norecurse -fileset $obj $files

# Set 'sources_1' fileset file properties for remote files
# set file "$origin_dir/my_first_microblaze/my_first_microblaze.srcs/sources_1/bd/microblaze_system_1/microblaze_system_1.bd"
set file "$origin_dir/bd_sources/microblaze_system_1.bd"
set file [file normalize $file]
set file_obj [get_files -of_objects [get_filesets sources_1] [list "*$file"]]
set_property "generate_synth_checkpoint" "0" $file_obj

# set file "$origin_dir/my_first_microblaze/my_first_microblaze.srcs/sources_1/bd/microblaze_system_1/hdl/microblaze_system_1_wrapper.vhd"
set file "$origin_dir/hdl_sources/microblaze_system_1_wrapper.vhd"
set file [file normalize $file]
set file_obj [get_files -of_objects [get_filesets sources_1] [list "*$file"]]
set_property "file_type" "VHDL" $file_obj


# Set 'sources_1' fileset file properties for local files
# None

# Set 'sources_1' fileset properties
set obj [get_filesets sources_1]
set_property "top" "microblaze_system_1_wrapper" $obj

# Create 'constrs_1' fileset (if not found)
if {[string equal [get_filesets -quiet constrs_1] ""]} {
  create_fileset -constrset constrs_1
}

# Set 'constrs_1' fileset object
set obj [get_filesets constrs_1]

# Add/Import constrs file and set constrs file properties
# set file "[file normalize "$origin_dir/my_first_microblaze/my_first_microblaze.srcs/constrs_1/new/constraint1.xdc"]"
set file "[file normalize "$origin_dir/constrs_1/constraint1.xdc"]"
set file_added [add_files -norecurse -fileset $obj $file]
# set file "$origin_dir/my_first_microblaze/my_first_microblaze.srcs/constrs_1/new/constraint1.xdc"
set file "$origin_dir/constrs_1/constraint1.xdc"
set file [file normalize $file]
set file_obj [get_files -of_objects [get_filesets constrs_1] [list "*$file"]]
set_property "file_type" "XDC" $file_obj

# Set 'constrs_1' fileset properties
set obj [get_filesets constrs_1]
# set_property "target_constrs_file" "$orig_proj_dir/my_first_microblaze.srcs/constrs_1/new/constraint1.xdc" $obj
set_property "target_constrs_file" "$origin_dir/constrs_1/constraint1.xdc" $obj

# Create 'sim_1' fileset (if not found)
if {[string equal [get_filesets -quiet sim_1] ""]} {
  create_fileset -simset sim_1
}

# Set 'sim_1' fileset object
set obj [get_filesets sim_1]
# Empty (no sources present)

# Set 'sim_1' fileset properties
set obj [get_filesets sim_1]
set_property "top" "microblaze_system_1_wrapper" $obj

# Create 'synth_1' run (if not found)
if {[string equal [get_runs -quiet synth_1] ""]} {
  create_run -name synth_1 -part xc7z020clg484-1 -flow {Vivado Synthesis 2014} -strategy "Vivado Synthesis Defaults" -constrset constrs_1
} else {
  set_property strategy "Vivado Synthesis Defaults" [get_runs synth_1]
  set_property flow "Vivado Synthesis 2014" [get_runs synth_1]
}
set obj [get_runs synth_1]

# set the current synth run
current_run -synthesis [get_runs synth_1]

# Create 'impl_1' run (if not found)
if {[string equal [get_runs -quiet impl_1] ""]} {
  create_run -name impl_1 -part xc7z020clg484-1 -flow {Vivado Implementation 2014} -strategy "Vivado Implementation Defaults" -constrset constrs_1 -parent_run synth_1
} else {
  set_property strategy "Vivado Implementation Defaults" [get_runs impl_1]
  set_property flow "Vivado Implementation 2014" [get_runs impl_1]
}
set obj [get_runs impl_1]

# set the current impl run
current_run -implementation [get_runs impl_1]

puts "INFO: Project created:my_first_microblaze"
