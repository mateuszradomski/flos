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
m :: flos-tf-base λ ../../flos/flos .

  ┏━━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━━━┓
  ┃  Thread  ┃  Files  ┃   Read   ┃  Token   ┃  Parse   ┃  Build   ┃  Render  ┃  Overall   ┃
  ┃          ┃         ┃   MB/s   ┃   MB/s   ┃   MB/s   ┃   MB/s   ┃   MB/s   ┃    MB/s    ┃
  ┣━━━━━━━━━━╋━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━━━┫
  ┃    T0    ┃    957  ┃   1010.5 ┃    493.8 ┃    851.5 ┃    610.1 ┃    779.5 ┃     140.6  ┃
  ┃    T1    ┃    942  ┃   1072.2 ┃    515.5 ┃    768.9 ┃    668.6 ┃    834.8 ┃     145.6  ┃
  ┃    T2    ┃    839  ┃    955.6 ┃    434.4 ┃    674.7 ┃    592.3 ┃    704.9 ┃     126.0  ┃
  ┃    T3    ┃   1043  ┃   1127.6 ┃    524.4 ┃    906.9 ┃    679.9 ┃    818.7 ┃     151.8  ┃
  ┃    T4    ┃   1020  ┃   1154.7 ┃    502.1 ┃    865.1 ┃    701.1 ┃    808.2 ┃     149.8  ┃
  ┃    T5    ┃   1067  ┃   1257.7 ┃    536.3 ┃    928.8 ┃    683.3 ┃    877.8 ┃     157.8  ┃
  ┃    T6    ┃   1118  ┃   1143.1 ┃    546.5 ┃    943.7 ┃    707.6 ┃    874.5 ┃     158.2  ┃
  ┃    T7    ┃   1013  ┃   1187.5 ┃    512.9 ┃    882.8 ┃    674.0 ┃    816.5 ┃     150.8  ┃
  ┃    T8    ┃    985  ┃    930.9 ┃    476.6 ┃    827.6 ┃    643.6 ┃    759.9 ┃     137.9  ┃
  ┣━━━━━━━━━━╋━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━━━┫
  ┃  ∑ Total ┃   8984  ┃   1089.6 ┃    504.8 ┃    846.9 ┃    662.4 ┃    808.3 ┃     146.5  ┃
  ┗━━━━━━━━━━┻━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━━━┛

Formatted 8984 files in 175ms (1284 MB/s)
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
