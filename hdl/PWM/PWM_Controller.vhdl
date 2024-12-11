library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity PWM_Controller is 
	generic (
		CLK_PERIOD		: time := 20 ns;
		W_DUTY_CYCLE	: integer := 22; -- 22.21;
		W_PERIOD			: integer := 13  -- 13.7
	);
	port (
		clk			: in	std_logic;
		rst			: in	std_logic;
		-- PWM repitition period in milliseconds;
		-- datatype (W.F) is individually assigned
		period		: in	unsigned(W_PERIOD - 1 downto 0);
		-- PWM duty cycle between [0 1]; out-of-range values are hard-limited
		-- datatype (W.F) is individually assigned
		duty_cycle	: in	unsigned(W_DUTY_CYCLE - 1 DOWNTO 0);
		output		: out	std_logic := '0'
	);
end entity PWM_Controller;

architecture PWM_Controller_arch of PWM_Controller is
	constant system_clock_frequency : integer := 1 sec / CLK_PERIOD; -- 50,000,000 for 20ns

	signal PWM_output : std_ulogic := '0';
	signal high_counter : integer := 0;
	signal period_counter : integer := 0;
	
begin


	modulator : process(clk, rst)
	begin

		if (rst = '1') then
			PWM_output <= '1';
			period_counter <= to_integer(shift_right(to_unsigned(system_clock_frequency, 32) * period, 7)) - 1;
			if (duty_cycle > "1000000000000000000000") then
				high_counter <= to_integer(shift_right(to_unsigned(system_clock_frequency, 32) * period, 7)) - 1;
			else
				high_counter <= to_integer(shift_right(to_unsigned(system_clock_frequency, 32)* period * duty_cycle, 28)) - 1;
			end if;
		elsif (rising_edge(clk) and period_counter = 0) then
			period_counter <= to_integer(shift_right(to_unsigned(system_clock_frequency, 32) * period, 7)) - 1;
			if (duty_cycle > "1000000000000000000000") then
				high_counter <= to_integer(shift_right(to_unsigned(system_clock_frequency, 32) * period, 7)) - 1;
			else
				high_counter <= to_integer(shift_right(to_unsigned(system_clock_frequency, 32)* period * duty_cycle, 28)) - 1;
			end if;
			PWM_output <= '1';
		elsif (rising_edge(clk) and high_counter > 0) then
			period_counter <= period_counter - 1;
			high_counter <= high_counter - 1;
			PWM_output <= '1';
		elsif (rising_edge(clk)) then
			period_counter <= period_counter - 1;
			PWM_output <= '0';
		end if;
	end process modulator;
	
	output_logic: process(clk, rst)
	begin
		if (rising_edge(clk)) then
			output <= PWM_output;
		end if;
	end process output_logic;
	
end architecture;