# Nori Exec

A Windows-first C++17 project for a **standalone Luau tooling environment**. This repository starts with reusable utility APIs and a console host; it does not inject into the Roblox client or bypass its protections.

## Included in this starter

- Sandboxed workspace filesystem: `readfile`, `writefile`, `appendfile`, `isfile`, `isfolder`, `listfiles`, `makefolder`, `delfile`, `delfolder`.
- Shared string environment for a standalone host (`getgenv)-style storage).
- Base64 encode/decode utilities.
- Console output helpers.
- Path traversal checks and basic tests.
- CMake build and GitHub Actions CI.

## Build

Requires CMake 3.20+ and a C++17 compiler.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Windows, Visual Studio 2022 with the **Desktop development with C++** workload is suitable.

## Project status

This is a foundation, not a drop-in Roblox executor and not a claim of UNC/sUNC compliance. Functions that depend on Roblox's private runtime, client injection, or anti-cheat evasion are intentionally not implemented. A future standalone Luau integration can be added using the official Luau project and its documented APIs.

## Layout

- `src/main.cpp` — sandboxed utility API and small demo.
- `CMakeLists.txt` — build and test configuration.
- `.github/workflows/build.yml` — Windows CI.

## Utility APIs currently implemented

- Workspace-scoped files: `readfile`, `writefile`, `appendfile`, `isfile`, `isfolder`, `listfiles`, `makefolder`, `delfile`, `delfolder`.
- File helpers: `getfilesize`, `copyfile`, `renamefile`.
- Shared standalone key/value environment: `getgenv()`.
- Encoding: `base64encode`, `base64decode`.
- Console: `rconsoleprint`, `rconsoleclear`, `rconsoleinfo`, `rconsolewarn`, `rconsoleerr`.
- Metadata: `identifyexecutor`, `getexecutorname`, `getexecutorversion`.
- Windows clipboard: `getclipboard`, `setclipboard`.

These are C++ utility methods, not yet registered as Luau globals. Clipboard methods are Windows-only. The self-test covers basic file operations, environment storage, Base64 round-tripping, and path traversal rejection.
