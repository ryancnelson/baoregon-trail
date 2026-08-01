#!/bin/bash
# tools/build_lock.sh -- acquire/release helpers for MEMO_BUILD_LOCK_PROTOCOL.md.
# Usage:
#   source tools/build_lock.sh && acquire_build_lock && { make clean && make test; release_build_lock; }

acquire_build_lock() {
    while [ -f .build-lock ]; do
        if [ -n "$(find .build-lock -mmin +2 2>/dev/null)" ]; then
            echo "stale .build-lock, clearing"; rm -f .build-lock; break
        fi
        sleep 5
    done
    echo "$$ $(whoami) $(date -u +%FT%TZ)" > .build-lock
}

release_build_lock() {
    rm -f .build-lock
}
