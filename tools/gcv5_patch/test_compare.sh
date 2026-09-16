#!/bin/bash
# Side-by-side: dispatcher power read (hal_power path) vs my version query
set -e
TOKEN=$(curl -s -X POST http://127.0.0.1:5000/login \
    -H 'Content-Type: application/json' \
    -d '{"username":"admin","password":"admin"}' \
    | python3 -c "import sys,json;print(json.load(sys.stdin)['token'])")

echo "--- A. dispatcher /power/read (hal_power path) ---"
curl -s --max-time 20 -X POST http://127.0.0.1:5000/power/read \
    -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' \
    -d '{"name":"VCC","metric":"voltage"}'
echo
echo "--- B. my /power_board/version (same _spi_exchange64) ---"
curl -s --max-time 20 http://127.0.0.1:5000/power_board/version \
    -H "Authorization: Bearer $TOKEN"
echo
echo "--- C. dispatcher /power/read again ---"
curl -s --max-time 20 -X POST http://127.0.0.1:5000/power/read \
    -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' \
    -d '{"name":"VCC","metric":"voltage"}'
echo
echo "--- D. my /power_board/version again ---"
curl -s --max-time 20 http://127.0.0.1:5000/power_board/version \
    -H "Authorization: Bearer $TOKEN"
echo
