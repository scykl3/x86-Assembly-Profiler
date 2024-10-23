# x86-Assembly-Profiler
A lightweight tool designed to analyze the execution time of x86 assembly instructions within a program, colleting runtime profiling data to show where most execution time is spent at both the function and assembly instruction level

## Features
- **Function-level profiling**: Displays the top functions where most execution time is spent
- **Instruction-level profiling**: Shows the exact assembly instructions that take the most time
- **Integration with `nm` and `objdump`**: Allows mapping program counters to functions and instructions for a detailed breakdown
- **Frequency-based execution timing**: Collects data in real time using `profil()`

### Prerequisites
- **GCC** (via MinGW-w64) or another C compiler
- Basic Unix environment with the proper utilities (`readlink`, `profil`, etc.)
- **nm** and **objdump** from GNU Binutils for function and instruction man pages

### Building and Running
1. **Clone the repository** and navigate to the directory:
   ```bash
   git clone https://github.com/scykl3/x86-Assembly-Profiler.git
   cd x86-Assembly-Profiler
   ```
   Remember to have the program you want to profile in this directory
2. **Include the Profiler Functions**
 Before you run your program, you'll need to modify the source code to include the histogram profiling functions.

 Add the following lines to the **beginning** and **end** of the `main()` function (or around the portion of code you want to analyze):

 ```c
extern void initialize_tracker();
extern void output_tracker();

 int main() {
     initialize_tracker();  // Start collecting data

     // Your program here

     output_tracker();  // Output the data at the end
     return 0;
 }
```
3. **Compiling**
   ```bash
   gcc -g -o your_program your_program.c tracker.c
   gcc -o profiler profiler.c
   ```
   A `.txt` will then be created, that can be passed into `profiler.c`
   
   ```bash
   profiler your_program_freq_table.txt
   ```
4. **Outputs**
The generated file will look something like this:
```bash
...
0x45b86a    60ms
0x45b870    1120ms
0x45b878    10ms
0x45b87c    30ms
...
```

`profile.c` will also produce an output that will look something like this:
```
Profile: your_program
Top 10 functions:
ith   Function             Time(ms)   (%)
1    : __func1__            10320      48.63%
2    : __func2__            3200       15.08%
3    : __func3_             2890       13.62%
4    : main                 1280       6.03%
5    : __func4__            1190       5.61%
6    : __func5__            620        2.92%
7    : __func6__            520        2.45%
8    : __func7__            340        1.60%
9    : __func8__            220        1.04%
10   : __func9__            200        0.94%

Top 10 functions Assembly:

1: __func1__          10320ms      48.63%

Assembly Instructions for function __func1__:
0x45b840:       f3 0f 1e fa           endbr64                          550ms
0x45b844:       89 f8                 mov    %edi,%eax                 120ms
0x45b846:       31 d2                 xor    %edx,%edx                 190ms
0x45b848:       62 a1 fd 00 ef c0     vpxorq %xmm16,%xmm16,%xmm16      10ms
0x45b84e:       09 f0                 or     %esi,%eax                 0ms
0x45b850:       25 ff 0f 00 00        and    $0xfff,%eax               530ms
...
...
                      
2: __func2__          3200ms      15.08%

Assembly Instructions for function __func2__:
0x402f0a:       f3 0f 1e fa           endbr64                          10ms
0x402f0e:       55                    push   %rbp                      10ms
0x402f0f:       48 89 e5              mov    %rsp,%rbp                 0ms
0x402f12:       48 83 ec 20           sub    $0x20,%rsp                0ms
...
...
```

   
