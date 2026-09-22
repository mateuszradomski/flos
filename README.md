# flos

A Solidity formatter written in C. No dependencies beyond a C compiler and pthreads. Capable of formatting at over a gigabyte per second on a modern CPU.

- **Parallel.** File discovery, tokenizing, parsing, and printing take advantage of your multicore CPU. On a cold cache your SSD is the slow part: reading the files off disk takes longer than formatting them.
- **Written from scratch.** The work queue, the lexer, the parser, and the pretty-printer are all in this repository. There is nothing to install. Running it on a Kindle is just one clang run away.
- **A real pretty-printer.** flos parses the file into a full syntax tree and renders it with a Wadler-style document algorithm, so line breaks and grouping are decided by the structure of the code rather than by heuristics.
- **Safe to run on a large codebase.** Built with the aim to be not only fast but correct. Verified across a large sample of real world contracts.

## Benchmark

```
$ flos -v .

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

M2 Pro, 16 GB RAM, best of 10 runs. The per-thread columns are the throughput of each stage in isolation; the last line is wall-clock throughput for the whole run.
On a cold file cache assume half of that throughput.

## Building and running

```bash
git clone https://github.com/mateuszradomski/flos
cd flos
bash build.sh
./flos <paths...>
```

Paths can be `.sol` files or directories. Directories are walked recursively and files are formatted in place.

```
Options:
  -v, --verbose           Print per-thread metrics after formatting.
  --sanity-check          Reruns the formatting to ensure stable results.
  --line-length=<N>       Maximum line width before wrapping (default: 120).
  --indent-width=<N>      Number of spaces per indent level (default: 4).
  -T=<N>, --threads=<N>   Number of worker threads.
  -h, --help              Show this help message and exit.
```
