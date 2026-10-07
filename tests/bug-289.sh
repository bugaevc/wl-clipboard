#!/usr/bin/env bash
set -e

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"

# Point these to your locally built binaries, NOT the system ones
WL_COPY="${ROOT}/build/src/wl-copy"
WL_PASTE="${ROOT}/build/src/wl-paste"

echo "Testing patched wl-copy with PHP content..."

# 1. Pipe the PHP payload into your newly built wl-copy without the -t flag
echo "<?php" | $WL_COPY

# 2. Extract the offered MIME types
OFFERED_TYPES=$($WL_PASTE --list-types)

# 3. Assert that generic text/plain is now in the list
if echo "$OFFERED_TYPES" | grep -q "^text/plain$"; then
    echo "✅ PASS: wl-copy successfully offered 'text/plain' for PHP!"
    
    # 4. Final validation: Ensure the actual paste command works as text/plain
    PASTED_CONTENT=$($WL_PASTE -t text/plain)
    if [ "$PASTED_CONTENT" = "<?php" ]; then
        echo "✅ PASS: wl-paste successfully retrieved the payload."
        exit 0
    else
        echo "❌ FAIL: Payload corruption."
        exit 1
    fi
else
    echo "❌ FAIL: text/plain is STILL missing. Your patch didn't work."
    echo "Offered types were:"
    echo "$OFFERED_TYPES"
    exit 1
fi
