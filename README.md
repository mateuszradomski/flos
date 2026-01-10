# flos

A hand-rolled, zero-dependency, multi-threaded C solidity formatter breaking GB/s of throughput.

- Raw performance: uses a parallel pipeline that splits file discovery, tokenizing, parsing, and printing across all your available CPU cores. It will saturate your I/O and CPU, and it will not apologize
- Hand-Rolled with Zero-Dep: No package manager. No dependency hell. Everything is from scratch—the multi-threaded work queue, the memory arenas, the parser, the pretty-printer. Just a C compiler and pthreads. That's it.
- Intelligent Pretty-Printing: This isn't a naive line-by-line indenter. _flos_ builds a full Abstract Syntax Tree and uses a proper document-based rendering algorithm (based on Wadler algorithm) to intelligently break lines and group elements. The output is beautiful and consistent
- It Won't Break Your Build: Tested on a mountain of real-world code to ensure it's not just fast, but unbreakable. Your code's syntax is sacred.

## Rough benchmarks

Expect better benchmarks in the future, but to deliver on the GB/s promise, here it is:

```
$ flos .
Thread : Files : Read : Token : Parse : Build : Render : Overall
    T0     962   1220     517     852     629      826       149
    T1     983   1029     496     782     628      787       140
    T2    1019   1100     506     820     616      814       144
    T3    1019   1032     497     840     627      792       142
    T4    1051   1336     554     903     696      871       160
    T5    1090   1005     501     888     661      820       146
    T6    1001   1007     504     775     604      792       139
    T7     953   1016     502     849     606      780       141
    T8     906   1079     485     786     578      745       136
Formatted 8984 files in 178ms (1265 MB/s)
```

Ran on M2 Pro, 16GB RAM, best of 10 runs.

## Usage

```bash
# Clone
git clone https://github.com/mateuszradomski/flos
cd flos

# Compile
bash build.sh

# Use
./flos <path>
```

Currently it assumes 120 columns per line and 4 spaces per indentation level.
Configurable soon.
