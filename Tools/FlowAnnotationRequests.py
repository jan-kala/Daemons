# Supporting script for pulling the information from the Joiner&Dispatcher

from ast import Bytes
import socket
import struct
import sys
import json

def main():

    if len(sys.argv) != 7:
        print("Incorrect number of arguments")
        return

    # Get the info from arguments
    commPort = int(sys.argv[1])
    srcIP = sys.argv[2] 
    srcPort = int(sys.argv[3])
    dstIP = sys.argv[4]
    dstPort = int(sys.argv[5])
    timestamp = int(sys.argv[6])
    
    message = {
        "srcIp" : srcIP,
        "srcPort" : srcPort,
        "dstIp" : dstIP,
        "dstPort" : dstPort,
        "timestamp" : timestamp
    }

    jsonMessage= json.dumps(message)
    jsonMessageBytes = jsonMessage.encode("utf-8")

    messageSize = len(jsonMessageBytes)
    messageSize = socket.htonl(messageSize)
    dataSize = struct.pack("I", messageSize)
    data = b"".join([dataSize, jsonMessageBytes])

    sockFd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sockFd.connect(("127.0.0.1", commPort))

    # Send data to Dispatcher
    sockFd.send(data)

    recvDataLen = sockFd.recv(4)
    recvDataLen = struct.unpack("I", recvDataLen)[0]
    recvDataLen = socket.ntohl(recvDataLen)

    recvData = sockFd.recv(recvDataLen)
    response = json.loads(recvData.decode("utf-8"))

    print(json.dumps(response, indent=4))

    sockFd.close()

if __name__ == "__main__":
    main()
