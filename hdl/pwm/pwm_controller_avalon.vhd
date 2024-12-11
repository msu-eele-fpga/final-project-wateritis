library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use IEEE.std_logic_textio.all;

entity pwd_controller_avalon is
	port (
		clk : in std_ulogic;
		rst : in std_ulogic;
		-- avalon memory-mapped slave interface
		avs_read : in std_logic;
		avs_write : in std_logic;
		avs_address : in std_logic_vector(1 downto 0);
		avs_readdata : out std_logic_vector(31 downto 0);
		avs_writedata : in std_logic_vector(31 downto 0);
		-- external I/O; export to top-level
		period : in unsigned(25 downto 0);
		duty_cycle : in unsigned( 14 downto 0);
		red_out : out std_logic;
		green_out : out std_logic;
		blue_out : out std_logic
	);
end entity pwd_controller_avalon;


Architecture reg of pwd_controller_avalon is
	signal red_duty_cycle			: std_logic_vector(31 downto 0) := "00000000000000000001000000000000";
	signal green_duty_cycle 		: std_logic_vector(31 downto 0) := "00000000000000000001000000000000";
	signal blue_dudty_cycle			: std_logic_vector(31 downto 0) := "00000000000000000001000000000000";
	signal peri							: std_logic_vector(31 downto 0) := "00000000001100000000000000000000";
	
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
	
	begin
	
	-- Red LED
	S0: pwm_controller port map(clk => clk, rst => rst, period => unsigned(peri(25 downto 0)), duty_cycle => unsigned(red_duty_cycle(14 downto 0)), output => red_out);
	
	-- Green LED
	S1: pwm_controller port map(clk => clk, rst => rst, period => unsigned(peri(25 downto 0)), duty_cycle => unsigned(green_duty_cycle(14 downto 0)), output => green_out);
	
	--BLue LED
	S2: pwm_controller port map(clk => clk, rst => rst, period => unsigned(peri(25 downto 0)), duty_cycle => unsigned(blue_dudty_cycle(14 downto 0)), output => blue_out);
	
	
avalon_register_read : process (clk)
			begin
				if rising_edge (clk) and avs_read = '1' then
					case avs_address is
						when "00" => avs_readdata <= red_duty_cycle;
						when "01" => avs_readdata <= green_duty_cycle;
						when "10" => avs_readdata <= blue_dudty_cycle;
						when "11" => avs_readdata <= peri;
						when others => avs_readdata <= (others => '0');
					end case;
				end if;
		end process;
	
		avalon_register_write : process (clk , rst)
			begin
				if rst = '0' then
					red_duty_cycle 	<= "00000000000000000001000000000000";
					green_duty_cycle 	<= "00000000000000000001000000000000";
					blue_dudty_cycle 	<= "00000000000000000001000000000000";
					peri 					<= "00000000001100000000000000000000";
					
				elsif rising_edge (clk) and avs_write = '1' then
					case avs_address is
						when "00" => red_duty_cycle  			<= avs_writedata;
						when "01" => green_duty_cycle 		<= avs_writedata;
						when "10" => blue_dudty_cycle 		<= avs_writedata;
						when "11" => peri 						<= avs_writedata;
						when others => null;
						end case;
				end if;
		end process;

end Architecture reg;