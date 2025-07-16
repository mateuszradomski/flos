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
    T0     972    971     483     834     565      588       129
    T1    1004   1008     518     846     567      560       131
    T2    1152   1157     526     888     601      628       140
    T3    1034    932     519     864     575      613       133
    T4    1002   1120     503     772     594      614       134
    T5    1112   1278     533     922     603      602       142
    T6     954   1196     487     810     517      575       128
    T7     943    880     460     789     532      530       120
    T8     805    733     423     703     478      475       107
Formatted 8978 files in 198ms (1134 MB/s)
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
