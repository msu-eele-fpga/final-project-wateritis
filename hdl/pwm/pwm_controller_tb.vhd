library ieee;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use IEEE.std_logic_textio.all;
use IEEE.numeric_std_unsigned.all;

entity pwm_controller_tb is
end entity pwm_controller_tb;

architecture testbench of pwm_controller_tb is

  constant CLK_PERIOD : time := 10 ns;

	component pwm_controller is
		generic (
			CLK_PERIOD 	: time 	  := 20 ns
		);
		port (
			clk : in std_logic;
			rst : in std_logic;
			-- PWM repetition period in milliseconds;
			-- datatype (W.F) is individually assigned
			period : in unsigned(25 downto 0);
			-- PWM duty cycle between [0 1]; out-of-range values are hard-limited
			-- datatype (W.F) is individually assigned
			duty_cycle : in unsigned( 14 downto 0);
			output : out std_logic
		);
	end component pwm_controller;

  signal clk_tb                  : std_logic := '0';
  signal rst_tb                  : std_logic := '1';
  signal period_tb          	 : unsigned(25 downto 0);
  signal duty_cycle_tb           : unsigned( 14 downto 0);
  signal output_tb		 : std_logic;


begin

  dut : component pwm_controller
  port map(
	clk => clk_tb,
	rst => rst_tb,
	period => period_tb,
	duty_cycle =>  duty_cycle_tb,
	output => output_tb
  );

  clk_make : process is
  begin

    clk_tb <= not clk_tb;
    wait for CLK_PERIOD / 2;

  end process clk_make;

    period_tb <= "00001000000000000000000000";
    duty_cycle_tb <= "001000000000000";

end architecture testbench;
