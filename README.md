# TFTP

[![Ubuntu](https://github.com/buracchi/file-transfer-over-udp/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/buracchi/file-transfer-over-udp/actions/workflows/ubuntu.yml)

Client/server file transfer application using a connectionless network service 
(SOCK_DGRAM type of the Berkeley sockets API, i.e. UDP as transport layer protocol).

Term paper of Internet and Web Engineering course.

## Overview

The application provides the following services:
- Unauthenticated client-server connection
- Reliable file transfer
- A communication protocol between the client and the server providing the exchange of two types of messages:
  - command messages: sent from the client to the server to request the execution of the various operations
  - response messages: sent from the server to the client in response to a command with the outcome of the operation
- the execution in the user space without requiring root privileges

### Server

The server (multithreaded) listening on a default port, provide the following services:
- Send the response message to the `list` command with the filelist (the list of file names available for sharing)
- Send the response message to the `get` command with the required file, if any, or an error message
- Sending the response message to the `put` command with the outcome of the operation
- Configure the listening port

### Client

The client provides the user with the following functions:
- Show the files available on the server (via the `list` command)
- Download a file from the server (via the `get` command)
- Upload a file on the server (via the `put` command)

### Reliable transmission

The exchange of messages takes place using an unreliable communication service.
In order to ensure the correct sending/receiving of messages and files both 
clients and servers implement at the application level the Go-Back N protocol.

During the tests to simulate the loss of messages over the network (a very 
unlikely event in a local network let alone when client and server are running 
on the same host), it is assumed that each message is discarded by the sender 
with probability $p$.

The size $N$ of the dispatch window, the probability $p$ of loss of messages,
and the duration $T$ of the timeout, are three configurable constants and the 
same for all processes. In addition to the use of a fixed timeout, it is 
possible to choose the use of a value for the adaptive timeout calculated 
dynamically based on the evolution of the observed network delays.
