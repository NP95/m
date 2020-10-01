# Dependencies

The following external dependencies must be satisfied to run the
project.

* Verilator (version >= 4.035)
* A compiler supporting C++17

# Instructions

# Basic build instructions

``` shell
# Clone project
git clone git@github.com:stephenry/m.git
pushd m
# Check out external dependencies
git submodule init
git submodule update
# Build project
mkdir build
pushd build
# Configure project
cmake ..
# Build project
cmake --build .
# Run regression using the CTEST driver (takes minutes)
ctest .
```

# Build with VCD

``` shell
# Enable waveform dumping (slows simulation)
cmake -DOPT_VCD_ENABLE=ON ..
```

# Build with logging

``` shell
# Enable logging (slows simulation)
cmake -DOPT_LOGGING_ENABLE=ON ..
```

# Run a test

``` shell
# Run smoke test
./tb/tests/smoke

# Run fully randomized regression
./tb/tests/regress
```

# Notes:

* Completed solution is located in: [m.sv](./rtl/m.sv)
* Ancillary RTL is located in: [libv](./libv)
* Testbench is located in: [tb](./tb)
* A basic smoke-test is located at: [smoke.cc](./tb/tests/smoke.cc)
* A fully-randomized verification environment is located at [regress.cc](./tb/tests/regress.cc)
* Design has been verified over 1M randomized packets (as per the constraints).

# Discussion:

The RTL solution consists as follows:

* Packets arrive at m.sv where they are latched by an input register.
* A simple FSM (fsm_PROC) is implemented to maintain the context of the word within the packet (as demarcated by the SOP and EOP fields).
* Matching logic (match_type_PROC) is implemented to match the 'type' field within a packet. The match operation is appropriately qualified on the validity of the bytes within the word.
* Matching logic (match_symbol_PROC) is implemented to match the 'symbol' field within the packet. The problem solution was not explicit on the alignment requirements of the symbol field and it has been assumed that the match is performed on an 8B boundary (the match cannot take place over successive cycles).
* A packet is considered 'matched' only if both the 'type' and at least one 'symbol' field has been detected within the packet body at the permissible locations.
* The match operands are presented to the RTL on the SOP of the packet and may therefore change on a per-packet basis. This can be hardwired into the RTL fairly easily by using an elaboration-time constant at the cost of some (probably small) area and frequency advantage.
* The initial latch at the input incurs one cycle of latency; without knowlege of the logic before the M module, it is unclear whether this is strictly necessary and can perhaps be removed. The match operation is carried out purely combinatorially over one cycle. Some latency is incurred across the asynchronous boundary between the NET and HOST clock domains. This latency is a function of the relative clock frequencies of the design and is an unavoidable artifact of the requirement to synchronize control signals between two, mutually-asynchronous clock domains. In the context of the verification environment, where the HOST clock operates at twice the frequency of the NET clock, the overall latency from input to output is approximately 4-5 NET clock cycles. Within a latency constrained environment, clock-domain crossing is generally inadvisible, if not otherwise avoidable.
* Verification of the RTL has been carried out in [regress.cc](./tb/tests/regress.cc). In this test, 1000 randomized verification contexts are created and within each 1000 randomized packets are issued to the RTL. The verification environment is self-checking and is therefore capable of indentifing errors that may be encountered during the simulation. By default, and for speed, the verification environment does not emit a waveform. A waveform (VCD) can be emitted by enabling the OPT_VCD_ENABLE option during project configuration. The resultant VCD can subsequently be viewed using either a free, open-source viewer (such as GTKWave), or a commerical offering.
