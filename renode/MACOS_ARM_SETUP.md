# Renode on Apple Silicon (macOS ARM64) -- real setup, verified

**Current status (verified 2026-08-06, Renode 1.16.1 on macOS 26.5.2,
arm64/T6020): works.** Renode ships a real, official, dedicated
Apple Silicon build -- this is NOT a "runs under Rosetta" situation,
and it is NOT a known-broken configuration as of this version.

## Install (Homebrew tap -- the real, recommended path)

```
brew tap renode/tap
brew trust renode/tap        # this fork of brew requires an explicit
                              # trust step for third-party taps; skip
                              # this line on stock Homebrew
brew install renode/tap/renode
```

This pulls the real `renode` formula (currently 1.16.1) plus its real
dependencies (`dotnet` 10.0, `gtk+3`, etc.) as prebuilt bottles --
no source build required. Confirmed working:

```
$ renode --version
Renode v1.16.1.15992
  build: d66b0c2a-202606120852
  build type: Release
  runtime: .NET 10.0.10
```

## Alternative: direct .dmg download

If you don't use Homebrew, the official releases page has a real,
dedicated Apple Silicon package:
https://github.com/renode/renode/releases/latest ->
`renode-1.16.1-dotnet.osx-arm64-portable.dmg` (nightly builds also
available at `builds.renode.io/renode-latest.osx-arm64-portable.dmg`).

## Headless / scripted usage (what this project actually needs)

For CI-style or scripted runs (no GUI, no interactive monitor), use:

```
renode --console --disable-xwt path/to/script.resc
```

`--disable-xwt` avoids the GTK/XWT GUI stack entirely -- confirmed
this avoids any GUI-related flakiness on this Mac. Without it, Renode
opens real windows (monitor console + any `showAnalyzer` terminals),
which is fine for interactive use but not appropriate for scripted
verification.

**Known real quirk observed on this Mac**: sending `quit` as the last
command in a `.resc` script did not reliably terminate the `renode`
process's `--console` output stream in our testing (the process
appeared to keep running past the `quit` line). Workaround: wrap the
whole invocation in a shell `timeout` instead of relying on an
in-script `quit`/`pause` sequence, e.g.:

```
timeout 10 renode --console --disable-xwt renode/bao1x_demo.resc
```

This is what `renode/build_and_run_demo.sh` and the verification runs
in `renode/README.md` actually use.

## What we did NOT need for this task

- No Rosetta/x86_64 emulation layer was needed -- the arm64-native
  `renode` binary ran directly.
- No manual `dotnet` SDK install was needed -- Homebrew's `dotnet`
  dependency (10.0.302) was sufficient.
- No GTK/XQuartz X11 setup was needed for headless (`--disable-xwt`)
  use.

## Real peripheral-generation tooling notes

`svd2repl` (used to generate `renode/bao1x.repl` from the real
Baochip-1x SVD) is **not** bundled with the Renode installation itself
-- it's a separate Rust tool that lives in `betrusted-io/xous-core`'s
own `svd2repl/` crate (built with `cargo build --release -p svd2repl`
from a clone of that repo; see `renode/README.md` for the exact real
commands used). Renode's own `tools/` directory (in the `renode/renode`
repo) has a *different*, PeakRDL-based `PeakRDL-repl` exporter that
takes SystemRDL/IP-XACT input, not raw SVD -- that is a different tool
for a different input format, not a drop-in replacement for the
`svd2repl` CLI this task's brief refers to.
