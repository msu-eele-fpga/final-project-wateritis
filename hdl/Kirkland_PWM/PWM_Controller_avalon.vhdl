----------------------------------------------------------------------------
-- Description:  LED pattern device which implements the state machine and the asynchronous conditioning conditions on the push button input.
----------------------------------------------------------------------------
-- Author:       Grant Kirkland
-- Company:      Montana State University
-- Create Date:  October 13, 2024
-- Revision:     1.0
----------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.math_real.all;
use ieee.numeric_std.all;

entity PWM_controller_avalon is 
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
		GPIO				: out	std_logic_vector(2 downto 0)
	);
end entity PWM_controller_avalon;

architecture PWM_controller_avalon_arch of PWM_controller_avalon is
	signal clk_period : time := 20 ns;
	signal duty_cycle_width : integer := 22;
	signal period_width : integer := 13;
	
	signal period_reg: std_ulogic_vector(31 downto 0) 		:= "00000000000000000000000010000000";
	signal red_dc_reg: std_ulogic_vector(31 downto 0) 		:= "00000000000100000000000000000000"; -- 50%
	signal grn_dc_reg: std_ulogic_vector(31 downto 0) 		:= "00000000000100000000000000000000"; -- 50%
	signal blu_dc_reg: std_ulogic_vector(31 downto 0) 		:= "00000000000100000000000000000000"; -- 50%
		
	component PWM_Controller is
		generic (
			CLK_PERIOD		: time := clk_period;
			W_DUTY_CYCLE	: integer := duty_cycle_width; -- 22.21;
			W_PERIOD			: integer := period_width  -- 13.7
		);
		port (
			clk			: in	std_logic;
			rst			: in	std_logic;
			period		: in	unsigned(W_PERIOD - 1 downto 0);
			duty_cycle	: in	unsigned(W_DUTY_CYCLE - 1 DOWNTO 0);
			output		: out	std_logic := '0'
		);
	end component PWM_Controller;
	
begin
	
	PWM00_Red : PWM_Controller
	port map (
		clk => clk,
		rst => rst,
		period => unsigned(period_reg(period_width - 1 downto 0)),
		duty_cycle => unsigned(red_dc_reg(duty_cycle_width - 1 DOWNTO 0)),
		output => GPIO(0)
	);
	
	PWM01_Green : PWM_Controller
	port map (
		clk => clk,
		rst => rst,
		period => unsigned(period_reg(period_width - 1 downto 0)),
		duty_cycle => unsigned(grn_dc_reg(duty_cycle_width - 1 DOWNTO 0)),
		output => GPIO(1)
	);
	
	PWM02_Blue : PWM_Controller
	port map (
		clk => clk,
		rst => rst,
		period => unsigned(period_reg(period_width - 1 downto 0)),
		duty_cycle => unsigned(blu_dc_reg(duty_cycle_width - 1 DOWNTO 0)),
		output => GPIO(2)
	);

	avalon_register_read : process(clk)
	begin
		if (rising_edge(clk) and avs_read = '1') then
			case avs_address is 
				when "00" => avs_readdata <= std_logic_vector(period_reg);
				when "01" => avs_readdata <= std_logic_vector(red_dc_reg);
				when "10" => avs_readdata <= std_logic_vector(grn_dc_reg);
				when "11" => avs_readdata <= std_logic_vector(blu_dc_reg);
				when others => avs_readdata <= (others => '0');
			end case;
		end if;
	end process avalon_register_read;

	
	avalon_register_write : process(clk, rst)
	begin
		if (rst = '1') then
			period_reg <= "00000000000000000000000010000000";
			red_dc_reg <= "00000000000100000000000000000000";
			grn_dc_reg <= "00000000000100000000000000000000";
			blu_dc_reg <= "00000000000100000000000000000000";
		elsif (rising_edge(clk) and avs_write = '1') then
			case avs_address is 
				when "00" => period_reg <= std_ulogic_vector(avs_writedata(31 downto 0));
				when "01" => red_dc_reg <= std_ulogic_vector(avs_writedata(31 downto 0));
				when "10" => grn_dc_reg <= std_ulogic_vector(avs_writedata(31 downto 0));
				when "11" => blu_dc_reg <= std_ulogic_vector(avs_writedata(31 downto 0));
				when others => null;
			end case;
		end if;
	end process;

end architecture PWM_controller_avalon_arch;