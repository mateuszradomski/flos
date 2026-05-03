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
  ┃    T0    ┃    830  ┃   1754.5 ┃    881.9 ┃    889.8 ┃    695.5 ┃    859.1 ┃     184.2  ┃
  ┃    T1    ┃    839  ┃   1798.8 ┃    886.4 ┃    885.8 ┃    720.2 ┃    863.0 ┃     186.6  ┃
  ┃    T2    ┃    843  ┃   1517.9 ┃    873.0 ┃    876.7 ┃    722.6 ┃    850.6 ┃     181.7  ┃
  ┃    T3    ┃    856  ┃   1791.8 ┃    844.0 ┃    865.9 ┃    650.8 ┃    837.0 ┃     177.6  ┃
  ┃    T4    ┃    820  ┃   1471.0 ┃    889.7 ┃    936.9 ┃    696.2 ┃    853.1 ┃     182.5  ┃
  ┃    T5    ┃    798  ┃   1671.1 ┃    832.3 ┃    849.9 ┃    631.4 ┃    766.2 ┃     170.5  ┃
  ┃    T6    ┃    835  ┃   1068.9 ┃    868.9 ┃    831.9 ┃    685.5 ┃    855.2 ┃     169.0  ┃
  ┃    T7    ┃    819  ┃   1475.1 ┃    863.5 ┃    852.2 ┃    720.0 ┃    826.5 ┃     178.3  ┃
  ┃    T8    ┃    881  ┃   1620.4 ┃    881.8 ┃    880.6 ┃    712.4 ┃    838.4 ┃     182.4  ┃
  ┣━━━━━━━━━━╋━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━━━┫
  ┃  ∑ Total ┃   7521  ┃   1542.8 ┃    869.1 ┃    874.2 ┃    692.0 ┃    838.3 ┃     179.2  ┃
  ┃     Part ┃         ┃    11.6% ┃    20.6% ┃    20.5% ┃    25.9% ┃    21.4% ┃            ┃
  ┗━━━━━━━━━━┻━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━━━┛

Formatted 7521 files in 230ms (1511 MB/s)
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
