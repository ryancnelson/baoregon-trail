#!/bin/bash
# Ralph loop: poll the baoregon-trail crew every 5 minutes, forever.
# Answers any pending questions with "keep going" and tells them to get back to work.

export MAESTRI_TERMINAL_ID="010EF33A-6D0E-44C7-B77D-A7121AA62775"
export MAESTRI_SOCKET="/var/folders/1b/80zgfzbn5_999wpy33r6xdf40000gn/T/maestri-91c168ed/maestri.sock"
export MAESTRI_CLI="/var/folders/1b/80zgfzbn5_999wpy33r6xdf40000gn/T/maestri-91c168ed/maestri"

LOG="/Volumes/T9/ryan-homedir/devel/baoregon-trail/ralph-loop.log"

log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >> "$LOG"
}

log "=== Ralph loop started (PID $$) ==="

while true; do
  for name in Woz Bunnie Duke baochip; do
    log "--- checking $name ---"
    out=$(maestri ask "$name" "Status check: if you have an open question or are waiting on a go-ahead, the answer is YES -- keep going, proceed with your next TDD cycle / next task without stopping to ask. Only message back if you hit a genuine blocker (missing info, cross-domain conflict, dangerous/irreversible action) that truly requires a human. Otherwise just continue working." 2>&1)
    log "$name reply (truncated): $(echo "$out" | tail -c 800)"
  done

  cd /Volumes/T9/ryan-homedir/devel/baoregon-trail 2>/dev/null
  commits=$(git log --oneline -5 2>&1)
  log "recent commits: $commits"

  log "sleeping 300s..."
  sleep 300
done
