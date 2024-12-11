----------------------------------------------------------------------------
-- Description:  Button debouncer, holding button output for a specified time
----------------------------------------------------------------------------
-- Author:       Grant Kirkland
-- Company:      Montana State University
-- Create Date:  September 11, 2024
-- Revision:     1.0
----------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity debouncer is
	generic(
		clk_period		: time := 20 ns;
		debounce_time	: time
	);
	port (
		clk				: in	std_ulogic;
		rst				: in	std_ulogic;
		input				: in	std_ulogic;
		debounced		: out	std_ulogic
	);
end entity debouncer;

architecture debouncer_arch of debouncer is 
	type state_Type is (ready, triggered);
	signal current_state : State_Type;
	
	constant wait_count:	integer := debounce_time / clk_period;
	signal count: 			integer := debounce_time / clk_period;
	signal hold_value:		std_ulogic := '0';

begin
	state_logic: process(clk, rst, input)
	begin
		if (rst = '1') then
			current_state <= ready;
			hold_value <= input;
			count <= wait_count - 1;
		elsif (rising_edge(clk)) then
			case (current_state) is
				when ready => 
					if (input = not hold_value) then 
						hold_value <= input; 
						current_state <= triggered;
					end if;
				when triggered => 
					if (count = 0) then
						if (input = hold_value) then 
							count <= wait_count - 1;
							current_state <= ready;
						else
							count <= wait_count - 1;
							hold_value <= input;							
						end if;
					else 
						count <= count - 1;
					end if;
			end case;
		end if;
	end process state_logic;

	debounced <= hold_value;

end architecture debouncer_arch;