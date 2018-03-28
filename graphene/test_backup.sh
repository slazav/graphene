#!/bin/bash -u

function assert(){
  if [ "$1" != "$2" ]; then
    printf "ERROR:\n"
    printf "  res:\n%s\n" "$1";
    printf "  exp:\n%s\n" "$2";
    exit 1
  fi
}

rm -rf test_backup
mkdir -p test_backup/a
mkdir -p test_backup/b

./graphene -d test_backup/a create db1
./graphene -d test_backup/a put db1 now 0.10
./graphene -d test_backup/a put db1 now 0.11
./graphene -d test_backup/a put db1 now 0.12

db_hotbackup -h test_backup/a -b test_backup/b

./graphene -d test_backup/a put db1 now 0.13

db_hotbackup -h test_backup/a -b test_backup/b -u

assert\
  "$(./graphene -d test_backup/a get db1)"\
  "$(./graphene -d test_backup/b get db1)"
