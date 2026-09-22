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

  ┏━━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━━━┓
  ┃  Thread  ┃  Files  ┃   Read   ┃  Token   ┃  Parse   ┃  Build   ┃  Render  ┃  Overall   ┃
  ┃          ┃         ┃   MB/s   ┃   MB/s   ┃   MB/s   ┃   MB/s   ┃   MB/s   ┃    MB/s    ┃
  ┣━━━━━━━━━━╋━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━━━┫
  ┃    T0    ┃    942  ┃   2574.5 ┃    901.6 ┃   1245.3 ┃    994.0 ┃   1035.5 ┃     234.1  ┃
  ┃    T1    ┃    938  ┃   2408.4 ┃    920.5 ┃   1188.9 ┃    940.2 ┃    994.5 ┃     226.7  ┃
  ┃    T2    ┃    938  ┃   2287.0 ┃    908.3 ┃   1179.1 ┃    936.3 ┃    981.1 ┃     223.5  ┃
  ┃    T3    ┃    935  ┃   2396.1 ┃    943.7 ┃   1241.2 ┃    994.5 ┃   1024.4 ┃     234.5  ┃
  ┃    T4    ┃    997  ┃   2739.7 ┃   1005.1 ┃   1331.9 ┃   1009.2 ┃   1084.0 ┃     248.5  ┃
  ┃    T5    ┃    963  ┃   2574.6 ┃    988.9 ┃   1286.7 ┃   1040.0 ┃   1053.7 ┃     244.7  ┃
  ┃    T6    ┃    930  ┃   2200.8 ┃    944.2 ┃   1218.7 ┃   1001.3 ┃   1008.7 ┃     231.3  ┃
  ┃    T7    ┃    934  ┃   2436.2 ┃    948.6 ┃   1241.2 ┃    987.8 ┃   1034.9 ┃     235.3  ┃
  ┃    T8    ┃   1084  ┃   2685.2 ┃   1020.7 ┃   1379.1 ┃   1114.4 ┃   1111.2 ┃     258.1  ┃
  ┣━━━━━━━━━━╋━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━━━┫
  ┃  ∑ Total ┃   8661  ┃   2472.5 ┃    953.4 ┃   1256.6 ┃   1001.5 ┃   1036.7 ┃     237.4  ┃
  ┃     Part ┃         ┃     9.6% ┃    24.9% ┃    18.9% ┃    23.7% ┃    22.9% ┃            ┃
  ┗━━━━━━━━━━┻━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━━━┛

Formatted 8661 files in 262ms (2072 MB/s)
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
Configure with `--line-length=<N>` and `--indent-width=<N>`.
