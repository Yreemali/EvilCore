![Status](https://img.shields.io/badge/build-non_deterministic-red)
![Language](https://img.shields.io/badge/language-C%2B%2B20-blue)
![UB](https://img.shields.io/badge/UB-critical-black)
![Compiler Stability](https://img.shields.io/badge/compiler-threatened-orange)


# EvilCore
Experimental compile-time adaptive execution framework written in cursed C++.


> ⚠️ **Warning:** This binary allocates executable memory, calls `ptrace` on itself,
> sends signals to its own PID, and forks on startup.  
> Your antivirus will not like this. That's expected.

> ⚠️ **Warning:** The author is not responsible for:
> - compiler crashes
> - hardware overheating
> - corrupted stack traces
> - psychological damage caused by reading the source code

## Overview

EvilCore is an experimental C++20 project focused on:
- compile-time SAT solving
- encrypted execution paths
- low-level Linux internals
- anti-analysis techniques
- template metaprogramming abuse
- UB-sensitive execution behavior

The repository intentionally explores compiler edge cases and non-trivial runtime behavior.

## Build

```bash
g++ -std=c++20 main.cpp -o evilcore
```

Compilation success is not guaranteed. Behavior may vary depending on:
- compiler version
- optimization level
- CPU architecture
- kernel version
- entropy conditions


## Reverse Engineering

Static and dynamic analysis may be complicated by:
- encrypted runtime logic
- inline assembly
- self-debugging behavior
- indirect execution paths
- compile-time generated structures

## License?
Do whatever you want. The compiler will judge you either way.

> **Good luck building it.**