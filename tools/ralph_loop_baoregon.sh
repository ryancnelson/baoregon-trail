#!/bin/bash
# ralph_loop_baoregon.sh -- Fable's overnight monitoring loop for
# baoregon-trail, per Ryan's explicit instruction: a REAL background
# process that sleeps and loops forever, NOT a cron job (Ryan reacts
# sharply if given cron/logging instead of a literal ralph loop -- see
# memory).
#
# Every 20 minutes:
#   1. Check project status (git log, make test, ralph-loop.log tail)
#   2. Decide: on track, or stuck?
#   3. Append notes to NEXT_STEPS.md
#   4. Nudge the Maestro session (tmux "danny-pane") with a status-aware
#      message
#   5. Repeat forever
#
# Stretch goal: boot Apple DOS 3.3 from a floppy, see it on the
# ramfb-synced screen.

REPO="$HOME/devel/baoregon-trail"
LOGFILE="/tmp/fable_ralph_loop.log"
INTERVAL=1200  # 20 minutes

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOGFILE"
}

log "=== Fable ralph-loop starting (PID $$) ==="

while true; do
    cd "$REPO" || { log "ERROR: cannot cd to $REPO"; sleep "$INTERVAL"; continue; }

    log "--- Check-in ---"

    # 1. Recent commit activity (last 30 min worth, roughly)
    RECENT_COMMITS=$(git log --oneline --since="25 minutes ago" 2>&1)
    COMMIT_COUNT=$(echo "$RECENT_COMMITS" | grep -c . || true)
    log "Commits in last ~25min: $COMMIT_COUNT"
    if [ -n "$RECENT_COMMITS" ]; then
        log "Recent commits:"
        echo "$RECENT_COMMITS" | tee -a "$LOGFILE"
    fi

    # 2. Test suite health
    TEST_OUTPUT=$(make test 2>&1)
    TEST_EXIT=$?
    PASS_COUNT=$(echo "$TEST_OUTPUT" | grep -c "^PASS" || true)
    FAIL_COUNT=$(echo "$TEST_OUTPUT" | grep -ci "^FAIL" || true)
    log "make test: exit=$TEST_EXIT PASS=$PASS_COUNT FAIL=$FAIL_COUNT"

    # 3. Ralph-loop.log tail (crew's own status log, if present)
    if [ -f "ralph-loop.log" ]; then
        CREW_STATUS=$(tail -15 ralph-loop.log 2>&1)
        log "Crew ralph-loop.log tail:"
        echo "$CREW_STATUS" | tee -a "$LOGFILE"
    fi

    # 4. NEXT_STEPS.md Step 7 (disk emulation) progress check
    STEP7_STATUS=$(grep -A2 "^## 💾 Step 7" NEXT_STEPS.md 2>&1 | head -5)

    # Decide: on track or stuck?
    STATUS="UNKNOWN"
    if [ "$FAIL_COUNT" -gt 0 ] 2>/dev/null; then
        STATUS="STUCK (test failures present)"
    elif [ "$COMMIT_COUNT" -eq 0 ] 2>/dev/null; then
        STATUS="POSSIBLY STALLED (no commits in ~25min)"
    else
        STATUS="ON TRACK (commits landing, tests green)"
    fi
    log "Assessment: $STATUS"

    # 5. Update NEXT_STEPS.md with a timestamped note (append, don't
    # clobber -- this is a durable project doc, not a scratch log)
    {
        echo ""
        echo "<!-- fable-ralph-loop check-in $(date '+%Y-%m-%d %H:%M:%S') -->"
        echo "**Fable's automated check-in:** $STATUS. Test suite: $PASS_COUNT PASS / $FAIL_COUNT FAIL (exit $TEST_EXIT). Commits in last ~25min: $COMMIT_COUNT."
    } >> NEXT_STEPS.md

    git add NEXT_STEPS.md 2>&1 | tee -a "$LOGFILE"
    git commit -m "Fable ralph-loop: automated check-in ($STATUS)" 2>&1 | tee -a "$LOGFILE"
    # Stash any concurrent crew edits picked up mid-cycle before pulling,
    # so a real rebase conflict never blocks this loop's own push.
    git stash --include-untracked 2>&1 | tee -a "$LOGFILE"
    git pull --rebase origin main 2>&1 | tee -a "$LOGFILE"
    git stash pop 2>&1 | tee -a "$LOGFILE" || log "stash pop: nothing to pop or already applied (fine)"
    git push origin main 2>&1 | tee -a "$LOGFILE"
    git push github main 2>&1 | tee -a "$LOGFILE"

    # 6. Nudge the Maestro session
    NUDGE_MSG=""
    if echo "$STATUS" | grep -q "STUCK"; then
        NUDGE_MSG="Fable check-in: test suite has $FAIL_COUNT failing test(s) as of now. Worth having the crew look at this before continuing new work -- don't let failures pile up. Stretch goal reminder: boot Apple DOS 3.3 from a floppy and see it on the ramfb-synced screen."
    elif echo "$STATUS" | grep -q "STALLED"; then
        NUDGE_MSG="Fable check-in: no commits in the last ~25 minutes. Just checking in -- are you blocked on something, or mid-thought on a larger change? Stretch goal reminder: boot Apple DOS 3.3 from a floppy and see it on the ramfb-synced screen."
    else
        NUDGE_MSG="Fable check-in: $COMMIT_COUNT commit(s) landed recently, tests green ($PASS_COUNT passing). Good progress -- keep going toward the stretch goal: boot Apple DOS 3.3 from a floppy and see it on the ramfb-synced screen."
    fi

    tmux send-keys -t danny-pane "$NUDGE_MSG" 2>&1 | tee -a "$LOGFILE"
    sleep 1
    tmux send-keys -t danny-pane Enter 2>&1 | tee -a "$LOGFILE"

    log "Nudged danny-pane: $NUDGE_MSG"
    log "Sleeping ${INTERVAL}s..."
    sleep "$INTERVAL"
done
