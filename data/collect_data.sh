#!/bin/bash
cd "$(dirname "$0")"

TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")

mkdir $TIMESTAMP && cd $TIMESTAMP

echo "timestamp, source,\"url\",\"data\"" > joined.csv
cat ../../HTTPDataReceiver/data/browser.csv >> ./joined.csv
cat ../../InterfaceMonitor/data/interface.csv >> ./joined.csv