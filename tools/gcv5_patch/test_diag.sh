#!/bin/bash
# Diagnose: deployed file hash, hal_power params, live SPI/M_INT test
cd /home/cat/gcv5_service
echo "--- deployed md5 ---"
md5sum core/power_board_update.py
echo "--- hal_power params ---"
grep -n 'spi_dev\|cs_port\|ready' hal/hal_power.py | head -4
echo "--- stray send/receive prints ---"
grep -rn 'print("send"\|print("receive"' core/ hal/ 2>/dev/null | head -4
echo "--- live test via service API ---"
TOKEN=$(curl -s -X POST http://127.0.0.1:5000/login \
    -H 'Content-Type: application/json' \
    -d '{"username":"admin","password":"admin"}' \
    | python3 -c "import sys,json;print(json.load(sys.stdin)['token'])")
echo -n "A. /power/read  : "
curl -s --max-time 20 -X POST http://127.0.0.1:5000/power/read \
    -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' \
    -d '{"name":"VCC","metric":"voltage"}'
echo
echo -n "B. /power_board/version : "
curl -s --max-time 20 http://127.0.0.1:5000/power_board/version \
    -H "Authorization: Bearer $TOKEN"
echo
echo -n "C. /power/read again : "
curl -s --max-time 20 -X POST http://127.0.0.1:5000/power/read \
    -H "Authorization: Bearer $TOKEN" -H 'Content-Type: application/json' \
    -d '{"name":"VCC","metric":"voltage"}'
echo
