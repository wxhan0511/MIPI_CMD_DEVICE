#!/bin/bash
# Reproduce: repeated version queries over HTTP
set -e
TOKEN=$(curl -s -X POST http://127.0.0.1:5000/login \
    -H 'Content-Type: application/json' \
    -d '{"username":"admin","password":"admin"}' \
    | python3 -c "import sys,json;print(json.load(sys.stdin)['token'])")

for i in 1 2 3 4 5; do
    echo "--- query $i ---"
    curl -s --max-time 30 http://127.0.0.1:5000/power_board/version \
        -H "Authorization: Bearer $TOKEN"
    echo
    sleep 1
done
