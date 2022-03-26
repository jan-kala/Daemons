#!/usr/bin/env -S python3 -u
# https://developer.mozilla.org/en-US/docs/Mozilla/Add-ons/WebExtensions/Native_messaging
# https://github.com/mdn/webextensions-examples/pull/157
# Note that running python with the `-u` flag is required on Windows,
# in order to ensure that stdin and stdout are opened in binary, rather
# than text, mode.

import os
import sys
import json
import struct
import datetime

sys.path.append("..")
import ProtobufMessages.Out.python.HTTPMessage_pb2

HEADERS = "tabId\turl"

def openLog(mode = "a"):
    current_dir = os.path.abspath(os.path.dirname(__file__))
    return open(current_dir + "/data/browser.csv", mode)

def closeLog(log_fd):
    log_fd.close()

def resetLog():
    log_fd = openLog("w")
    # log_fd.write("[{}] STARTING\n".format(datetime.datetime.now()))
    closeLog(log_fd)

def log(message):
    log_fd = openLog()
    # log_fd.write("[{}] ".format(datetime.datetime.now()))
    log_fd.write(message)
    log_fd.write("\n")
    closeLog(log_fd)



# Python 3.x version
# Read a message from stdin and decode it.
def getMessage():
    rawLength = sys.stdin.buffer.read(4)
    if len(rawLength) == 0:
        sys.exit(0)
    messageLength = struct.unpack('@I', rawLength)[0]
    message = sys.stdin.buffer.read(messageLength).decode('utf-8')
    return message

# Encode a message for transmission,
# given its content.
def encodeMessage(messageContent):
    encodedContent = json.dumps(messageContent).encode('utf-8')
    encodedLength = struct.pack('@I', len(encodedContent))
    return {'length': encodedLength, 'content': encodedContent}

# Send an encoded message to stdout
def sendMessage(encodedMessage):
    log("SendMessage\n")
    sys.stdout.buffer.write(encodedMessage['length'])
    sys.stdout.buffer.write(encodedMessage['content'])
    sys.stdout.buffer.flush()

try:
    resetLog()
    # log(HEADERS)
    while True:
        # just receive messages
        receivedMessage = getMessage()
        msg = json.loads(receivedMessage)["message"]
        
        ts = str(float(msg["timeStamp"]) / 1000)
        
        url = msg["url"]
        proto = ""
        if (url[0:7] == "http://"):
            proto = "HTTP"
            url = url[7:]
        elif (url[0:8] == "https://"):
            proto = "TLS"
            url = url[8:]

        log("{ts}, Browser, {proto},\"{url}\",\"{data}\"".format(ts=ts, url=url, data=msg, proto=proto))

except Exception as e:
    sys.stdout.buffer.flush()
    sys.stdin.buffer.flush()
    sys.exit(0)
