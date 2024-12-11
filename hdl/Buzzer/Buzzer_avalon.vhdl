----------------------------------------------------------------------------
-- Description:  Buzzer avalon support.
----------------------------------------------------------------------------
-- Author:       Grant Kirkland
-- Company:      Montana State University
-- Create Date:  December 09, 2024
-- Revision:     1.0
----------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.math_real.all;
use ieee.numeric_std.all;

entity Buzzer_avalon is 
	port (
		clk				: in	std_ulogic;
		rst				: in	std_ulogic;
		
		-- avalon memory-mapped slave interface
		avs_read			: in	std_logic;
		avs_write		: in	std_logic;
		avs_address		: in	std_logic_vector(1 downto 0);
		avs_readdata	: out	std_logic_vector(31 downto 0);
		avs_writedata	: in	std_logic_vector(31 downto 0);
		
		-- external I/O; export to top-level
		GPIO				: out	std_logic
	);
end entity Buzzer_avalon;

architecture Buzzer_avalon_arch of Buzzer_avalon is
	signal clk_period : time := 20 ns;
	signal duty_cycle_width : integer := 22;
	signal period_width : integer := 13;
	
	signal period_reg: std_ulogic_vector(31 downto 0) 		:= "00000000000000000000000010000000";
		
	component Buzzer is
		generic (
			CLK_PERIOD		: time := clk_period;
			W_PERIOD			: integer := period_width  -- 13.12
		);
		port (
			clk			: in	std_logic;
			rst			: in	std_logic;
			period		: in	unsigned(W_PERIOD - 1 downto 0);
			output		: out	std_logic := '0'
		);
	end component Buzzer;
	
begin
	
	PWM00_Red : Buzzer
	port map (
		clk => clk,
		rst => rst,
		period => unsigned(period_reg(period_width - 1 downto 0)),
		output => GPIO
	);
	
	-- Checks if read was flagged, if so, checks register and reads out data
	avalon_register_read : process(clk)
	begin
		if (rising_edge(clk) and avs_read = '1') then
			case avs_address is 
				when "00" => avs_readdata <= std_logic_vector(period_reg);
				when others => avs_readdata <= (others => '0');
			end case;
		end if;
	end process avalon_register_read;

	-- Checks if write was flagged, if so checks address and writes to appropriate register
	avalon_register_write : process(clk, rst)
	begin
		if (rst = '1') then
			period_reg <= "00000000000000000000000010000000";
		elsif (rising_edge(clk) and avs_write = '1') then
			case avs_address is 
				when "00" => period_reg <= std_ulogic_vector(avs_writedata(31 downto 0));
				when others => null;
			end case;
		end if;
	end process;

end architecture Buzzer_avalon_arch;