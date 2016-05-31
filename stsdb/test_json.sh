#!/bin/sh -u

# test json interface ./stsdb_json

function assert(){
  if [ "$1" != "$2" ]; then
    printf "ERROR:\n"
    printf "  res:\n%s\n" "$1";
    printf "  exp:\n%s\n" "$2";
    exit 1
  fi
}

###########################################################################

# remove all test databases
rm -f test*.db

# create and fill databases

assert "$(./stsdb -d . create test_1 DOUBLE)" ""
assert "$(./stsdb -d . create test_2 UINT16)" ""
assert "$(./stsdb -d . create test_3 TEXT)" ""

assert "$(./stsdb -d . put test_1 10 0.1 0.25)" ""
assert "$(./stsdb -d . put test_1 20 0.2 0.26)" ""
assert "$(./stsdb -d . put test_1 30 0.3 0.27)" ""

assert "$(./stsdb -d . put test_2 15 1 11)" ""
assert "$(./stsdb -d . put test_2 25 2 12)" ""
assert "$(./stsdb -d . put test_2 35 3 13)" ""

assert "$(./stsdb -d . put test_3 5 1st text msg)" ""
assert "$(./stsdb -d . put test_3 27 2st text msg)" ""
assert "$(./stsdb -d . put test_3 40 3st text msg)" ""


req='
{"panelId":3,
    "range":{"from":"1970-01-01T00:00:00.001Z","to":"1970-01-01T00:00:00.025Z"},
    "interval":"1ms",
    "targets":[
      {"refId":"A","target":"test_1"},
      {"refId":"B","target":"test_2:2"},
      {"refId":"C","target":"test_1:2"}
    ],
    "format":"json",
    "maxDataPoints":10
}'
ans='[{"target": "test_1", "datapoints": [[0.10000000000000001, 10], [0.20000000000000001, 20]]}, {"target": "test_2:2", "datapoints": [[0.0, 15], [0.0, 25]]}, {"target": "test_1:2", "datapoints": [[0.0, 10], [0.0, 20]]}]'
assert "$(printf "%s" "$req" | ./stsdb_json . /query)" "$ans"

# same but with larger interval - only one point redurned
req='
{"panelId":3,
    "range":{"from":"1970-01-01T00:00:00.001Z","to":"1970-01-01T00:00:00.025Z"},
    "interval":"15ms",
    "targets":[
      {"refId":"A","target":"test_1"},
      {"refId":"B","target":"test_2:2"},
      {"refId":"C","target":"test_1:2"}
    ],
    "format":"json",
    "maxDataPoints":10
}'
ans='[{"target": "test_1", "datapoints": [[0.10000000000000001, 10]]}, {"target": "test_2:2", "datapoints": [[0.0, 15]]}, {"target": "test_1:2", "datapoints": [[0.0, 10]]}]'
assert "$(printf "%s" "$req" | ./stsdb_json . /query)" "$ans"

# annotations
ann='"annotation": {"name": "test_3", "datasource": "Simple JSON Datasource",'\
' "iconColor": "rgba(255, 96, 96, 1)", "enable": true, "query": "#test"}'
req='
{"range":{"from":"1970-01-01T00:00:00.001Z","to":"1970-01-01T00:00:00.040Z"},
 "rangeRaw":{"from":"now-1h","to":"now"}, '$ann' }'
ans='[{"title": "1st text msg", "time": 5, '$ann'},'\
' {"title": "2st text msg", "time": 27, '$ann'},'\
' {"title": "3st text msg", "time": 40, '$ann'}]'
assert "$(printf "%s" "$req" | ./stsdb_json . /annotations)" "$ans"

# try to do query from text db:
req='
{"panelId":3,
    "range":{"from":"1970-01-01T00:00:00.001Z","to":"1970-01-01T00:00:00.025Z"},
    "interval":"15ms",
    "targets":[
      {"refId":"A","target":"test_3"}
    ],
    "format":"json",
    "maxDataPoints":10
}'
ans='{"error_type": "jsonxx", "error_message":"Can not do query from TEXT database. Use annotations"}'
assert "$(printf "%s" "$req" | ./stsdb_json . /query)" "$ans"

# try to get annotations from numeric DB
ann='"annotation": {"name": "test_1", "datasource": "Simple JSON Datasource",'\
' "iconColor": "rgba(255, 96, 96, 1)", "enable": true, "query": "#test"}'
req='
{"range":{"from":"1970-01-01T00:00:00.001Z","to":"1970-01-01T00:00:00.040Z"},
 "rangeRaw":{"from":"now-1h","to":"now"}, '$ann' }'
ans='{"error_type": "jsonxx", "error_message":"Annotations can be found only in TEXT databases"}'
assert "$(printf "%s" "$req" | ./stsdb_json . /annotations)" "$ans"


###########################################################################
# remove all test databases
rm -f test*.db
