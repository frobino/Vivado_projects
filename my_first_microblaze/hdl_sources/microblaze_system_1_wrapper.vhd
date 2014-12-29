--Copyright 1986-2014 Xilinx, Inc. All Rights Reserved.
----------------------------------------------------------------------------------
--Tool Version: Vivado v.2014.3.1 (lin64) Build 1056140 Thu Oct 30 16:32:04 MDT 2014
--Date        : Thu Nov 13 17:00:13 2014
--Host        : T440p running 64-bit Debian GNU/Linux testing/unstable
--Command     : generate_target microblaze_system_1_wrapper.bd
--Design      : microblaze_system_1_wrapper
--Purpose     : IP block netlist
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity microblaze_system_1_wrapper is
  port (
    Clk : in STD_LOGIC;
    reset_rtl : in STD_LOGIC
  );
end microblaze_system_1_wrapper;

architecture STRUCTURE of microblaze_system_1_wrapper is
  component microblaze_system_1 is
  port (
    Clk : in STD_LOGIC;
    reset_rtl : in STD_LOGIC
  );
  end component microblaze_system_1;
begin
microblaze_system_1_i: component microblaze_system_1
    port map (
      Clk => Clk,
      reset_rtl => reset_rtl
    );
end STRUCTURE;
