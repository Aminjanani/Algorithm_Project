library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity morse_tb is
end morse_tb;

architecture TB_ARCHITECTURE of morse_tb is

    component morse
    port(
        clk         : in  STD_LOGIC;
        input       : in  STD_LOGIC;
        outleddot   : out STD_LOGIC;
        outleddas   : out STD_LOGIC;
        outledsep   : out STD_LOGIC;
        state_iden1 : out STD_LOGIC;
        state_iden2 : out STD_LOGIC;
        output      : out STD_LOGIC_VECTOR(7 downto 0);
        counter_o   : out integer
    );
    end component;

    signal clk         : STD_LOGIC := '0';
    signal input       : STD_LOGIC := '1';
    signal outleddot   : STD_LOGIC;
    signal outleddas   : STD_LOGIC;
    signal outledsep   : STD_LOGIC;
    signal state_iden1 : STD_LOGIC;
    signal state_iden2 : STD_LOGIC;
    signal output      : STD_LOGIC_VECTOR(7 downto 0);
    signal counter_o   : integer;

    -- Clock period (100 MHz)
    constant clk_period : time := 10 ns;

    -- Match DUT thresholds (DUT: DOT=20, DASH=40, IDLE=80 cycles)
    constant DOT_TIME  : time := 200 ns; -- 20 cycles
    constant DASH_TIME : time := 500 ns; -- 50 cycles (safe > 40 cycles)
    constant IDLE_TIME : time := 1 us;   -- 100 cycles (safe > 80 cycles)

    signal test_case_num    : integer := 0;
    signal expected_output  : STD_LOGIC_VECTOR(7 downto 0) := (others => '0');

begin

    UUT : morse
        port map (
            clk         => clk,
            input       => input,
            outleddot   => outleddot,
            outleddas   => outleddas,
            outledsep   => outledsep,
            state_iden1 => state_iden1,
            state_iden2 => state_iden2,
            output      => output,
            counter_o   => counter_o
        );

    -- Clock generation
    clock_process: process
    begin
        clk <= '0';
        wait for clk_period/2;
        clk <= '1';
        wait for clk_period/2;
    end process;

    -- Stimulus + checking
    stimulus: process

        procedure press_button(duration: time) is
        begin
            -- drive on falling edge to avoid race with rising_edge sampling
            wait until falling_edge(clk);
            input <= '0';
            wait for duration;
            wait until falling_edge(clk);
            input <= '1';
        end procedure;

        procedure send_dot is
        begin
            press_button(DOT_TIME);
            wait for DOT_TIME;  -- intra-symbol gap
        end procedure;

        procedure send_dash is
        begin
            press_button(DASH_TIME);
            wait for DOT_TIME;  -- intra-symbol gap
        end procedure;

        procedure wait_for_decode is
        begin
            -- Give DUT enough time to hit IDLE timeout and run PROCESSING
            wait for IDLE_TIME + 200 ns;
        end procedure;

        procedure send_char(pattern: string; expected: std_logic_vector(7 downto 0)) is
        begin
            for i in pattern'range loop
                case pattern(i) is
                    when '.' => send_dot;
                    when '-' => send_dash;
                    when others => null;
                end case;
            end loop;

            expected_output <= expected;

            -- wait until end-of-character gap triggers timer_done -> PROCESSING
            wait for IDLE_TIME;

            -- wait a bit more so output has time to update
            wait_for_decode;

            -- ASSERT / CHECK
            assert output = expected
                report "FAIL test_case=" & integer'image(test_case_num) &
                       " expected(dec)=" & integer'image(to_integer(unsigned(expected))) &
                       " got(dec)=" & integer'image(to_integer(unsigned(output)))
                severity error;

            report "PASS test_case=" & integer'image(test_case_num) &
                   " got(dec)=" & integer'image(to_integer(unsigned(output)))
                severity note;
        end procedure;

        procedure short_press_should_be_ignored is
            variable before_val : std_logic_vector(7 downto 0);
        begin
            before_val := output;

            press_button(50 ns); -- shorter than DOT_TIME (200ns)
            wait for IDLE_TIME + 500 ns;

            assert output = before_val
                report "FAIL test_case=" & integer'image(test_case_num) &
                       " short press changed output!"
                severity error;

            report "PASS test_case=" & integer'image(test_case_num) &
                   " short press ignored"
                severity note;
        end procedure;

    begin
        -- Initialize
        input <= '1';
        wait for 100 ns;

        -- Test case 1: A (.-) => index 0
        test_case_num <= 1;
        send_char(".-", x"00");
        wait for 200 ns;

        -- Test case 2: B (-...) => index 1
        test_case_num <= 2;
        send_char("-...", x"01");
        wait for 200 ns;

        -- Test case 3: S (...) => index 18 = 0x12
        test_case_num <= 3;
        send_char("...", x"12");
        wait for 200 ns;

        -- Test case 4: 3 (...--) => index 29 = 0x1D
        test_case_num <= 4;
        send_char("...--", x"1D");
        wait for 200 ns;

        -- Test case 5: Invalid short press (ignored, output unchanged)
        test_case_num <= 5;
        short_press_should_be_ignored;
        wait for 200 ns;

        -- Test case 6: SOS sequence (S=0x12, O=0x0E, S=0x12)
        test_case_num <= 6;
        send_char("...", x"12");  -- S
        send_char("---", x"0E");  -- O
        send_char("...", x"12");  -- S

        -- End
        test_case_num <= 0;
        report "All tests finished." severity note;
        wait;
    end process;

end TB_ARCHITECTURE;
