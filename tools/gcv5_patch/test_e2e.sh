#!/bin/bash
# E2E test: login -> power_board endpoints (run on the board)
set -e
TOKEN=$(curl -s -X POST http://127.0.0.1:5000/login \
    -H 'Content-Type: application/json' \
    -d '{"username":"admin","password":"admin"}' \
    | python3 -c "import sys,json;print(json.load(sys.stdin)['token'])")

echo "--- /power_board/version ---"
curl -s http://127.0.0.1:5000/power_board/version -H "Authorization: Bearer $TOKEN"
echo
echo "--- /power_board/status ---"
curl -s http://127.0.0.1:5000/power_board/status -H "Authorization: Bearer $TOKEN"
echo
echo "--- /update_page http code ---"
curl -s -o /dev/null -w '%{http_code}\n' http://127.0.0.1:5000/update_page -H "Authorization: Bearer $TOKEN"
