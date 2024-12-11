----------------------------------------------------------------------------
-- Description:  Asynchronos signal synchronizer, behaving as two sequential d-flip-flops
----------------------------------------------------------------------------
-- Author:       Grant Kirkland
-- Company:      Montana State University
-- Create Date:  September 4, 2024
-- Revision:     1.0
----------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;


entity synchronizer is
	port (
		clk	: in	std_ulogic;
		async	: in	std_ulogic;
		sync	: out	std_ulogic
	);
	
end entity synchronizer;

architecture synchronizer_arch of synchronizer is

	signal not_sync_yet : std_ulogic;
	
begin 
	Synchronize : process(clk)
	begin
		if (rising_edge(clk)) then
			not_sync_yet <= async;
			sync <= not_sync_yet;
		end if;
	end process;
	
end architecture;
	