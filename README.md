# Custom Malloc (42 `malloc` Project)

A drop‑in replacement allocator implementing TINY / SMALL / LARGE zones with segregated free bins, block coalescing, and (bonus) global thread safety. Provides an introspection function `show_alloc_mem()` plus optional runtime statistics / formatting via environment variables.

---
## 1. Quick Start
```bash
# Build the shared library + tests/benchmarks
make

# Run unit tests (baseline libc + custom allocator builds)
make test
./test.out          # baseline tests using system malloc
./test_custom.out   # tests using this allocator

# Run performance comparison (bench + bench_custom, no rebuild if fresh)
make perf

# Clean objects / binaries
make clean      # objects only
make fclean     # objects + libraries + test/bench binaries
make re         # full rebuild
```

---
## 2. Library Artifacts
After a successful build you will have:
- `libft_malloc_<HOSTTYPE>.so`  (real shared object; HOSTTYPE = `$(uname -m)_$(uname -s)`)
- `libft_malloc.so` (symlink used for dynamic loading / LD_PRELOAD)

Both reside at the project root for simplicity.

---
## 3. Using the Allocator in Your Program
### 3.1 Link-Time Replacement
Compile your program against the library first on the link line:
```bash
gcc -I./includes your_prog.c -L. -lft_malloc -Wl,-rpath,. -o your_prog
```
The `-Wl,-rpath,.` ensures the runtime linker finds `libft_malloc.so` in the current directory.

### 3.2 LD_PRELOAD (No Recompile Needed)
Run any existing dynamically linked program with the custom allocator:
```bash
LD_PRELOAD=./libft_malloc.so ./your_prog [args]
```
This forces the dynamic loader to resolve `malloc/free/realloc` (and friends) to the versions inside this project.

> Tip: Use an absolute path for scripts: `LD_PRELOAD="$(pwd)/libft_malloc.so" ./your_prog`.

---
## 4. Headers and Public API
Include the project header in sources that directly call `show_alloc_mem()` (or future extended APIs):
```c
#include "malloc.h"
```
Exported symbols:
- `malloc(size_t)`
- `free(void*)`
- `realloc(void*, size_t)`
- `show_alloc_mem(void)`

(Additional internal helpers are intentionally not exported.)

---
## 5. Introspection: `show_alloc_mem()`
`show_alloc_mem()` prints all zones and their blocks in ascending address order. A canonical minimal mode is produced when no formatting environment variables are set.

Example call inside your program:
```c
#include "malloc.h"
int main(void) {
    char *p = malloc(42);
    show_alloc_mem();
    free(p);
    return 0;
}
```

Sample (simplified) output:
```
TINY  : 0x55d8a9e1e000 - 0x55d8a9e1f000 (4096 bytes)
  0x55d8a9e1e050 - 0x55d8a9e1e07a : 42 bytes
SMALL : 0x55d8a9e20000 - 0x55d8a9e24000 (16384 bytes)
LARGE : 0x7f2c58c00000 - 0x7f2c58c02000 (8192 bytes)
Total : 42 bytes
```

---
## 6. Environment Variables (Optional Formatting & Stats)
| Variable            | Values        | Effect |
|---------------------|---------------|--------|
| `MALLOC_COLOR`      | `1`           | Enable ANSI color output. |
| `MALLOC_SHOW_STATS` | `1`           | Append summary statistics (zone usage, fragmentation hints). |
| `MALLOC_SHOW_FREE`  | `1`           | Also list currently free blocks with size. |

Use them ad hoc:
```bash
MALLOC_COLOR=1 MALLOC_SHOW_STATS=1 ./test_custom.out
```
If unset, output remains plain and minimal (subject‑style).

---
## 7. Thread Safety (Bonus)
All allocator entry points (`malloc`, `free`, `realloc`, `show_alloc_mem`) acquire a global recursive `pthread` mutex. This provides basic safety for multi‑threaded programs. More granular locking (per zone/bin) could improve scalability; current design favors simplicity and correctness.

---
## 8. Design Overview (High Level)
- Zones acquired via `mmap` (anonymous, private). Types:
  - TINY  : payload size ≤ `TINY_MAX` (default 128)
  - SMALL : payload size ≤ `SMALL_MAX` (default 4096) and > TINY_MAX
  - LARGE : payload size > `SMALL_MAX` (one zone per large alloc)
- Each zone maintains a doubly-linked list of blocks.
- Free blocks also participate in segregated size-class bins for faster reuse.
- On `free`, adjacent free neighbors are coalesced before reinsertion into bins (prevents fragmentation / bin corruption).
- Large allocations are `mmap`'d individually and fully `munmap`'d on free.
- Alignment: All block payloads are 16‑byte aligned.
- Corruption detection: Per-block magic header + consistency checks when manipulating bins.

---
## 9. Benchmarks
Run both baseline and custom allocator benchmarks:
```bash
make perf
```
The target executes:
1. Baseline benchmark (`bench.out`) using system malloc.
2. Custom benchmark (`bench_custom.out`) linked with our allocator.
3. Prints timing via `/usr/bin/time`.

You can rerun without rebuilding (incremental correctness verified):
```bash
make perf
```
Only the execution section appears if nothing changed.

> NOTE: Debug flags (`-g3 -Wall -Wextra -Werror`) are enabled; no optimization flags are added by default. Expect the custom allocator to be slower than glibc in raw micro-benchmarks while correctness instrumentation is enabled.

---
## 10. Testing
Baseline vs custom tests share the same test harness; custom-specific cases (allocation class boundaries, coalescing, large zone path) reside in `tests/custom_tests.c`.
```bash
make test
./test.out
./test_custom.out
```
CUnit output reports passed assertions; non‑zero exit indicates failure.

---
## 11. Debugging Tips
| Scenario | Tip |
|----------|-----|
| Suspected leak | Instrument with `show_alloc_mem()` before exit. |
| Corruption / crash | Rebuild (already with `-g3`) and run under `gdb` or `valgrind ./test_custom.out`. |
| Performance hotspot | Temporarily add `-O2` to `CFLAGS` or create a `CFLAGS+=-O2` override on the make command line. |

---
## 12. Extending
Ideas for future bonus features:
- Per-bin or per-zone locks for improved multi-thread throughput.
- Defragmentation / background coalescer.
- Extended API: `show_alloc_mem_ex()` for JSON or machine-readable dumps.
- Allocation size histogram / sampling profiler hooks.

---
## 13. Limitations / Notes
- Global mutex can serialize heavy multi-thread allocations.
- No explicit `calloc`, `memalign`, `free` size introspection interface (only standard trio + show).
- Environment features are optional and not mandated by the base subject; they can be disabled by leaving variables unset.

---
## 14. License
No explicit license file yet. Add one (e.g. MIT) if you intend to distribute publicly.

---
## 15. Minimal Usage Recap
```bash
make                     # build allocator
LD_PRELOAD=./libft_malloc.so ./your_prog  # preload
./test_custom.out         # run project tests using custom allocator
show_alloc_mem()          # call within your code to inspect state
```

---
Feel free to adapt this README section as you refine the allocator. PRs / patches (if this becomes public) should mention performance trade‑offs and test coverage changes.
