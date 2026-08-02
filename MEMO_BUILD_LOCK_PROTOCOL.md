# Memo: build/test lock protocol — avoiding concurrent `make test`/`make clean` collisions

**From:** Ryan (via Hermes)
**To:** Danny Ocean + crew (baochip, Woz, Bunnie, Duke) + Ralph Loop automation
**Re:** A real, reproduced race condition on shared `make test` runs

## What happened

Ran `make clean && make test` and got a transient failure:

```
ld: open() failed, errno=2 (No such file or directory) for 'build/test_reset'
```

`build/` was empty at link time despite the Makefile's own `mkdir -p $(BUILD_DIR)`
rule being correct — the most likely explanation is a second `make test` (almost
certainly Ralph Loop's own automation, which runs on a ~5 minute cadence per
`ralph-loop.log`) ran concurrently and its own `make clean` deleted `build/`
mid-compile of this session's run. Re-running immediately afterward passed clean
(579/579) with no code changes — consistent with a timing race, not a real bug.

This is a real hazard now that multiple agents (this session + baochip/Woz/Bunnie/
Duke's own terminals + Ralph Loop's automated pass loop) are all working in the
same checkout of `~/devel/baoregon-trail/` and can legitimately run `make clean`/
`make test` at the same moment.

## Proposed protocol: a simple lockfile, checked-in convention

No new tooling needed — a flag file plus a Makefile guard is enough:

1. **Before running `make clean` or `make test`**, any agent (human or automated)
   checks for a lock file at repo root: `.build-lock`.
   - If it exists, **don't run** — wait and retry (Ralph Loop: back off one poll
     cycle; a human: just wait ~10s and re-check).
   - If it doesn't exist, create it (`echo "$$ $(whoami) $(date -u +%FT%TZ)" > .build-lock`),
     run the build/test, then remove it when done (`rm -f .build-lock`) —
     including on failure (use a trap/`finally`, not just the happy path).
2. **`.build-lock` is gitignored, never committed** — it's a local coordination
   file, not project state. Add `.build-lock` to `.gitignore` if not already
   covered by the existing `build/` ignore pattern.
3. **Stale-lock safety**: if a lock file is older than ~2 minutes (longer than
   any real `make test` run takes, per the ~9-15s runs observed this session),
   treat it as abandoned (a crashed process that didn't clean up) and remove it
   before proceeding, rather than waiting forever.

### Concrete shell snippet (works in any agent's terminal, no new deps)

```bash
# Acquire
while [ -f .build-lock ]; do
  if [ -n "$(find .build-lock -mmin +2 2>/dev/null)" ]; then
    echo "stale lock, clearing"; rm -f .build-lock; break
  fi
  sleep 5
done
echo "$$ $(whoami) $(date -u +%FT%TZ)" > .build-lock

# Run the actual build/test, always releasing the lock after
make clean && make test
rm -f .build-lock
```

### If someone prefers a git-native mechanism instead

A `git`-based alternative (branch-per-agent + no shared `build/` at all) would
also work but is more overhead than this warrants — the lockfile above is
sufficient given the actual failure mode observed (a filesystem race on a
shared `build/` directory, not a git merge/history problem). Recommend sticking
with the flag-file approach unless a real git-level collision shows up later.

## Ask

Ralph Loop's automation (and any of baochip/Woz/Bunnie/Duke's own terminal
sessions that run `make test`/`make clean` directly) should adopt the
acquire/release pattern above before their next build/test invocation. This is
cheap insurance — the actual test suite itself is fine; this only guards the
shared `build/` directory during concurrent runs.

---

## Addendum (2026-08-02): file-edit locks, not just build locks

**What happened:** three specialists (Bunnie, Duke, Woz) independently converged
on the same root-cause bug (a framebuffer stride mismatch surfaced by the new
text-mode renderer) and started editing the same files simultaneously —
`src/emulator_loop.h`, `tests/test_emulator_loop_copy_framebuffer.c`,
`tests/test_emulator_loop_framebuffer_bounds.c`. The `.build-lock` protocol above
only guards `make clean`/`make test` runs — it does **not** guard concurrent
*edits* to the same source file. `git log`/`git status` alone isn't a real-time
signal either: commits land after the fact, so by the time you see a collision
in git you've already stepped on someone's uncommitted work.

**Extended protocol — file-edit locks, same pattern as `.build-lock`:**

1. **Before editing a file you didn't just create**, check for a lock at
   `.file-locks/<sanitized-path>.lock` (e.g. `.file-locks/src__emulator_loop.h.lock`).
   - If it exists and isn't stale (>2 min old), don't edit that file right now —
     work on something else and check back, or ask the coordinator (Danny/Maestro)
     to resolve who owns it.
   - If it doesn't exist (or is stale), create it
     (`echo "$$ $(whoami) $(date -u +%FT%TZ) editing" > .file-locks/<name>.lock`),
     do your edit, commit, then remove the lock.
2. **`.file-locks/` is gitignored**, same as `.build-lock` — local coordination
   only, never committed.
3. **When a root-cause bug touches shared infrastructure files** (headers,
   core data structures, anything referenced by 3+ specialists' domains), the
   coordinator should explicitly assign ONE owner before dispatching parallel
   work, rather than letting multiple specialists independently converge on the
   same fix. This is a dispatch-hygiene responsibility, not just a locking
   mechanism — the fastest fix for a shared-file collision is not starting one
   in the first place.
4. **`git status --porcelain` before touching any file you didn't just create**
   is still good practice on top of the lock (belt-and-suspenders) — if you see
   unexpected uncommitted changes in a file you're about to touch, stop and
   check with whoever's editing it before proceeding, exactly as Bunnie/Duke/Woz
   correctly did tonight when they noticed the collision mid-edit.

This is intentionally lightweight (no new tooling, same flag-file pattern
already proven for builds) — the goal is a cheap, real-time signal, not a full
distributed-lock system.
