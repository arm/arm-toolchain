# How to use BOLT with our toolchain

BOLT is a post-link optimizer available in ATfL. To benefit from it you can use
the following tools:

* `llvm-bolt`
* `llvm-bolt-heatmap`
* `perf2bolt`

## Example optimization of a pathological case

To try this example, download and adapt our Telemetry repository. You can do
that with the following commands:

```
$ git clone https://git.gitlab.arm.com/telemetry-solution/telemetry-solution.git

$ cd telemetry-solution

$ git checkout ffaf62bd11d42213fe175fbe3d313a246c85535f

$ cd tools/ustress
```

As BOLT utilizes relocations, you must ensure their emission. In our example,
this requires the following modification with the `sed` command:


```
$ sed -i 's/^LINKER_FLAGS = -lm$/LINKER_FLAGS = -lm -Wl,--emit-relocs/' Makefile
```

Also, you must deactivate the assertions:

```
$ sed -i 's/^\(\s*assert.*\)$/\/\/\1/' l1i_cache_workload.c
```

Compile the example for a specific CPU. Available alternatives are:

* Neoverse N1 (`CPU=NEOVERSE-N1`)
* Neoverse N2 (`CPU=NEOVERSE-N2`)
* Neoverse V1 (`CPU=NEOVERSE-V1`)

The following example command compiles the example for Neoverse V1:

```
$ make CC=armclang CPU=NEOVERSE-V1 USE_C=1
```

You can verify, that the binary file contains relocations:

```
$ llvm-readelf --sections l1i_cache_workload | grep .rela.text
  [14] .rela.text        RELA            0000000000000000 0f34a0 0004e0 18   I 45  13  8
```

Record a performance profile from an example execution of this binary:

```
$ perf record -e cycles:u -- ./l1i_cache_workload 5
```

Convert this performance profile to the BOLT format using `perf2bolt`:

```
$ perf2bolt -p perf.data -o perf.boltdata --nl l1i_cache_workload
```

Optimize this binary with BOLT:

```
$ llvm-bolt \
  l1i_cache_workload \
  -o l1i_cache_workload.bolt \
  --data perf.boltdata \
  --dyno-stats \
  --print-profile-stats \
  -reorder-blocks=ext-tsp \
  -reorder-functions=hfsort \
  -split-functions \
  -split-all-cold \
  -split-eh
```

The name of the new optimized binary is `l1i_cache_workload.bolt`. You can use
the `repeat` command of the `csh` shell to obtain a set of comparable
statistics:

```
$ csh

% repeat 3 perf stat -e L1I_CACHE_REFILL,instructions -- ./l1i_cache_workload 5

% repeat 3 perf stat -e L1I_CACHE_REFILL,instructions -- ./l1i_cache_workload.bolt 5

% exit
```

Notice how much faster the second binary executes.

Using the `llvm-bolt-heatmap` tool, you can visualize the improvements with
heatmaps:

```
$ perf record -e cycles:u -- ./l1i_cache_workload 5

$ llvm-bolt-heatmap -p perf.data --nl l1i_cache_workload -o heatmap-orig.txt

$ perf record -o perf-bolt.data -e cycles:u -- ./l1i_cache_workload.bolt 5

$ llvm-bolt-heatmap -p perf-bolt.data --nl l1i_cache_workload.bolt -o heatmap-bolt.txt
```

Inspect both heatmaps and observe the difference:

```
$ less heatmap-orig.txt

$ less heatmap-bolt.txt
```

As these heatmaps are usually big, we suggest using the `aha` utility to
convert them into HTML pages and view them in a web browser:

```
$ aha -b -f heatmap-orig.txt heatmap-orig.htm

$ aha -b -f heatmap-bolt.txt heatmap-bolt.html
```
