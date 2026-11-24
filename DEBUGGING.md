# Debugging crashes

NTgCalls is native code, so when it crashes the Python, Node.js or Go traceback usually stops at the call into the library. This page explains how to collect a native backtrace when reporting a crash, and how maintainers turn it into file and line numbers with the debug symbols published for every release.

## Reporting a crash

Open an issue with the **Crash report** template, which asks for:

- the NTgCalls version (and Py-TgCalls version, if you use it), the language version, the OS and the CPU architecture;
- the steps or the smallest script that reproduces the crash;
- a link to the native backtrace, collected as below.

Backtraces are long, so upload them to a paste service such as [Pastebin](https://pastebin.com/) or [BatBin](https://batbin.me/) and share the link in the issue.

### Linux

Start your program with this command:

```bash
gdb -q -batch -iex "set confirm off" \
      -iex "set pagination off" \
      -iex "handle SIGPIPE pass nostop noprint" \
      -iex "handle SIGINT pass nostop noprint" \
      -iex "set logging enabled on" \
      -iex "set debuginfod enabled on" \
      -ex run -ex bt -ex "info sharedlibrary" \
      --args python3 YOUR_SCRIPT.py
```

Use your program as usual until it crashes. You don't need to type anything: when it crashes, gdb prints the backtrace (`bt`) and where each library was loaded (`info sharedlibrary`), which the maintainers need to find the exact lines, then exits.

Everything gdb printed is also saved to `gdb.txt` in the folder where you ran the command: upload that file. You don't need to download any debug symbols, the installed package is enough.

What the options do:

- `handle SIGPIPE` and `handle SIGINT` let the program receive these signals without gdb stopping on them, so it only stops on the real crash;
- `set logging enabled on` writes everything gdb prints to `gdb.txt`, so nothing is lost when copying from the terminal; your program's own output isn't included;
- `set debuginfod enabled on` downloads the debug symbols of Python and of the system libraries from your distribution, so their frames show names too.

If the crash only happens in production, enable core dumps instead and open the core after the crash. On systems with systemd:

```bash
coredumpctl gdb
(gdb) set logging enabled on
(gdb) bt
(gdb) info sharedlibrary
```

### macOS

```bash
lldb -b -o run -o "bt all" -- python3 main.py
```

### Windows

Capture a crash dump with [ProcDump](https://learn.microsoft.com/sysinternals/downloads/procdump) and attach it to the issue (zip it if it's large):

```powershell
procdump -e -ma -x . python.exe main.py
```

## Reading a crash report

Save the reporter's gdb output (their `gdb.txt` or the paste) to a file and run, with the version they used:

```bash
python3 scripts/symbolize_backtrace.py gdb.txt --version 4.0.0-beta01
```

The script downloads the debug symbols of that version from the GitHub release, keeps them in `~/.cache/ntgcalls/symbols`, and prints the backtrace again with the file and line under every NTgCalls frame:

```
#11 0x00007fffe7252fd8 in ntgcalls::signaling::messages::Message::type () from .../ntgcalls.cpython-312-x86_64-linux-gnu.so
        at ntgcalls/src/signaling/messages/message.cpp:12
#12 0x00007fffe71fa79d in ntgcalls::instances::P2PCall::process_signaling_data () from .../ntgcalls.cpython-312-x86_64-linux-gnu.so
        at ntgcalls/src/instances/p2p_call.cpp:248
```

- The platform comes from the name of the Python module. For the Node.js and C libraries, add `--platform` with the reporter's platform, for example `--platform linux-x86_64`.
- If the script warns that some frames resolved to a different function than gdb printed, the version or platform is wrong: ask the reporter which version they ran.
- Frames without a line, such as the WebRTC ones, come from prebuilt code that has no debug symbols.
- `--symbols` takes a symbols archive, folder or `.debug` file you already have, instead of downloading it.

The script needs `addr2line`, from binutils or LLVM.

## Debug symbols

The published packages don't contain the debug information, to keep them small. On Linux the libraries keep their function names, so a backtrace already shows which functions were running; file and line numbers come from the debug symbols.

Every GitHub release has one archive of debug symbols per platform, `ntgcalls.<platform>_symbols.zip`, for example `ntgcalls.linux-x86_64_symbols.zip`. Each archive has one folder per package:

| Folder | Package | Linux | macOS | Windows |
|---|---|---|---|---|
| `python/` | the PyPI wheels, one file per Python version | `ntgcalls.cpython-312-x86_64-linux-gnu.so.debug` | `ntgcalls.cpython-312-darwin.so.dSYM` | `ntgcalls.cp312-win_amd64.pyd/ntgcalls.pdb` |
| `node/` | the npm package | `ntgcalls.node.debug` | `ntgcalls.node.dSYM` | `ntgcalls.node/ntgcalls.pdb` |
| `c/` | the shared C library used by Go and the desktop bindings | `libntgcalls.so.debug` | `libntgcalls.dylib.dSYM` | `ntgcalls.dll/ntgcalls.pdb` |

Always use the symbols of the exact version that crashed: they only match the binary they were built with.

### Linux core dumps

If a reporter sends a core dump instead, install the same package version and copy its `.debug` file next to the library, in the same folder. gdb then finds it on its own through the debug link stored in the library:

```bash
cp ntgcalls.cpython-312-x86_64-linux-gnu.so.debug "$(python3 -c 'import ntgcalls, os; print(os.path.dirname(ntgcalls.__file__))')/"
gdb python3 core
(gdb) thread apply all bt
```

### macOS

Load the `.dSYM` into lldb next to the crash report or core:

```bash
(lldb) target symbols add ntgcalls.cpython-312-darwin.so.dSYM
(lldb) bt all
```

### Windows

Add the folder that contains the matching `ntgcalls.pdb` to the symbol path, then open the dump. In WinDbg:

```
.sympath+ C:\symbols\python\ntgcalls.cp312-win_amd64.pyd
.reload
!analyze -v
```

In Visual Studio, add the same folder under Tools, Options, Debugging, Symbols.

## Local builds

Builds on your machine keep the debug information inside the library, so debuggers and IDEs work without extra steps. CI splits it out because the `SPLIT_DEBUG_SYMBOLS` CMake option is on when the `CI` environment variable is set; pass `-DSPLIT_DEBUG_SYMBOLS=ON` to try it locally, and the symbols land in `symbols/`.
