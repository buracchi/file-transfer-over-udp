# RFCs Compliance Matrix

<table>
	<th colspan="3">RFC 1350 - TFTP (Trivial File Transfer Protocol)</th>
	<tr>
		<td colspan="3">1. Purpose
	</tr>
	<tr>
		<td>
		<td>Reading files from a remote server support.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Writing files into a remote server support.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Data transmission use 8-bit bytes.
		<td>✅
	</tr>
	<tr>
		<td>
		<td>Netascii transfer mode (8 bit ascii with the modifications specified in "Telnet Protocol Specification") support.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Octet transfer mode (raw 8 bit bytes) support.
		<td>❌
	</tr>
	<tr>
		<td colspan="3">2. Overview of the Protocol
	</tr>
	<tr>
		<td>
		<td>A transfer begins with a request from the client to read or write a file, which also serves to request a connection.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>If the server grants the request, the connection is opened and the file is sent in fixed length blocks of 512 bytes.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Each data packet contains one block of data, and must be acknowledged by an acknowledgment packet before the next packet can be sent.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A data packet of less than 512 bytes signals termination of a transfer.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>If a packet gets lost in the network, the intended recipient will timeout and may retransmit his last packet (which may be data or an acknowledgment), thus causing the sender of the lost packet to retransmit that lost packet.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>An error is signalled by sending an error packet.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>An error packet is not acknowledged, and not retransmitted.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Timeouts are used to detect termination when the error packet has been lost.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Not being able to satisfy the request (e.g., file not found, access violation, or no such user) causes a termination error
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Receiving a packet which cannot be explained by a delay or duplication in the network (e.g., an incorrectly formed packet) causes a termination error
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Losing access to a necessary resource (e.g., disk full or access denied during a transfer) causes a termination error
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Receving a packet with incorrect source port causes a non-termination error
		<td>❌
	</tr>
	<tr>
		<td>
		<td>When receiving a packet with incorrect source port an error packet is sent to the originating host
		<td>❌
	</tr>
	<tr>
		<td colspan="3">3. Relation to other Protocols
	</tr>
	<tr>
		<td>
		<td>TFTP is implemented on top of UDP.
		<td>✅
	</tr>
	<tr>
		<td>
		<td>TFTP packets have an optional header (LNI, ARPA header, etc.) to allow them through the local transport medium, an Internet header, a Datagram header, and a TFTP header.
		<td>✅
	</tr>
	<tr>
		<td>
		<td>The order of the contents of a packet will be: local medium header, if used, Internet header, Datagram header, TFTP header, followed by the remainder of the TFTP packet. (This may or may not be data depending on the type of packet as specified in the TFTP header.)
		<td>✅
	</tr>
	<tr>
		<td>
		<td>The source and destination port fields of the Datagram header are used by TFTP and the length field reflects the size of the TFTP packet.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The transfer identifiers (TID's) used by TFTP are passed to the Datagram layer to be used as ports.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The transfer identifiers (TID's) used by TFTP must be between 0 and 65,535.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The TFTP header consists of a 2 byte opcode field which indicates the packet's type (e.g., TFTP_OPCODE_DATA, TFTP_OPCODE_ERROR, etc.).
		<td>❌
	</tr>
	<tr>
		<td colspan="3">4. Initial Connection Protocol
	</tr>
	<tr>
		<td>
		<td>A transfer is established by sending a request.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A TFTP_OPCODE_WRQ request is used to write onto a foreign file system.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A TFTP_OPCODE_RRQ request is used to read from a foreign file system.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>If the reply to a read request is an error packet, then the request has been denied.
		<td>❌
	</tr>
	<tr>
		<td>
        <td>If the reply to a read request is the first data packet for read, then the request has been accepted.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The positive response to a write request is an acknowledgment packet with block number zero.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>An acknowledgment packet that is not the positive response of a write request will contain the block number of the data packet being acknowledged.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Each data packet has associated with it a block number.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Block numbers are consecutive and begin with one.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>To establish a connection each end chooses a TID for itself, to be used for the duration of that connection.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The TID's chosen for a connection should be randomly chosen, so that the probability that the same number is chosen twice in immediate succession is very low.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Every packet has associated with it the two TID's of the ends of the connection, the source TID and the destination TID.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The TID's are stored in the UDP source and destination ports.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A requesting host chooses its source TID and sends its initial request to the known TID 69 decimal on the serving host.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Under normal operations, the server chooses its source TID and sends its response using its TID as its source TID and the TID chosen for the previous message by the requestor as its destination TID.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The two chosen TID's are then used for the remainder of the transfer.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>During the transfer the hosts should make sure that the source TID of a received packet matches the previously agreed value.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>If a source TID does not match, the packet should be discarded as erroneously sent from somewhere else and an error packet should be sent to the source of the incorrect packet, while not disturbing the transfer.
		<td>❌
	</tr>
	<tr>
		<td colspan="3">5. TFTP Packets
	</tr>
	<tr>
		<td>
		<td>TFTP_OPCODE_RRQ (read request, opcode = 1) packet type support.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>TFTP_OPCODE_WRQ (write request, opcode = 2) packet type support.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>TFTP_OPCODE_DATA (data, opcode = 3) packet type support.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>TFTP_OPCODE_ACK (acknowledgment, opcode = 4) packet type support.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>TFTP_OPCODE_ERROR (error, opcode = 5) packet type support.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The TFTP header of a packet contains the associated opcode.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>TFTP_OPCODE_RRQ packets (opcode 1) have the following format: <li> Opcode (2 bytes) <li> Filename (variable-length string) <li> 0x0 (1 byte) <li> Mode (variable-length string) <li> 0x0 (1 byte)
		<td>❌
	</tr>
	<tr>
		<td>
		<td>TFTP_OPCODE_WRQ packets (opcode 2) have the following format: <li> Opcode (2 bytes) <li> Filename (variable-length string) <li> 0x0 (1 byte) <li> Mode (variable-length string) <li> 0x0 (1 byte)
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Filename is a netascii sequence terminated by a zero byte.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Mode field specifies "netascii" or "octet" modes (case insensitive) terminated by a zero byte.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A host which receives netascii mode data must translate the data to its own format.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Octet mode is used to transfer a file that is in the 8-bit format of the machine from which the file is being transferred.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>If a host receives a octet file and then returns it, the returned file must be identical to the original.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Sender and recipient may operate in different modes.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>TFTP_OPCODE_DATA packets (opcode 3) have the following format: <li> Opcode (2 bytes) <li> Block number (2 bytes) <li> Data (n bytes)
		<td>❌
	</tr>
	<tr>
		<td>
		<td>TFTP_OPCODE_DATA packets have a block number and data field.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The block numbers on data packets begin with one and increase by one for each new block of data.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>An implementation should detect duplicated packets via the block number.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The data field is from zero to 512 bytes long.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>If the data field is 512 bytes long, the block is not the last block of data.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>If the data field is from zero to 511 bytes long, it signals the end of the transfer.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>All packets other than duplicate TFTP_OPCODE_ACK's and those used for termination are acknowledged unless a timeout occurs.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Sending a TFTP_OPCODE_DATA packet is an acknowledgment for the first TFTP_OPCODE_ACK packet of the previous TFTP_OPCODE_DATA packet.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The TFTP_OPCODE_WRQ and TFTP_OPCODE_DATA packets are acknowledged by TFTP_OPCODE_ACK or TFTP_OPCODE_ERROR packets, while TFTP_OPCODE_RRQ and TFTP_OPCODE_ACK packets are acknowledged by TFTP_OPCODE_DATA or TFTP_OPCODE_ERROR packets.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>TFTP_OPCODE_ACK packets (opcode 4) have the following format: <li> Opcode (2 bytes) <li> Block number (2 bytes)
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The block number in an TFTP_OPCODE_ACK echoes the block number of the TFTP_OPCODE_DATA packet being acknowledged.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A TFTP_OPCODE_WRQ is acknowledged with an TFTP_OPCODE_ACK packet having a block number of zero.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>TFTP_OPCODE_ERROR packets (opcode 5) have the following format: <li> Opcode (2 bytes) <li> ErrorCode (2 bytes) <li> ErrMsg (variable-length string) <li> 0x0 (1 byte)
		<td>❌
	</tr>
	<tr>
		<td>
		<td>An TFTP_OPCODE_ERROR packet can be the acknowledgment of any other type of packet.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The error code is an integer indicating the nature of the error.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The error message is intended for human consumption, and should be a string in netascii terminated with a zero byte.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Error handling for codes 0 (Not defined, see error message (if any).) are implemented
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Error handling for codes 1 (File not found.) are implemented
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Error handling for codes 2 (Access violation.) are implemented
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Error handling for codes 3 (Disk full or allocation exceeded.) are implemented
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Error handling for codes 4 (Illegal TFTP operation.) are implemented
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Error handling for codes 5 (Unknown transfer ID.) are implemented
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Error handling for codes 6 (File already exists.) are implemented
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Error handling for codes 7 (No such user.) are implemented
		<td>❌
	</tr>
	<tr>
		<td colspan="3">6. Normal Termination
	</tr>
	<tr>
		<td>
		<td>Transfer ends with a TFTP_OPCODE_DATA packet containing 0 to 511 bytes.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Final TFTP_OPCODE_DATA packet is acknowledged like other TFTP_OPCODE_DATA packets.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The host sending the final TFTP_OPCODE_ACK wait for a long timeout before terminating in oreder to retransmit the final TFTP_OPCODE_ACK if lost.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Sender of the final TFTP_OPCODE_DATA packet should retransmit until acknowledged or timeout.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Successful completion is indicated by reception of the final TFTP_OPCODE_ACK.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Timeout or TFTP_OPCODE_DATA sender unwillingness to retransmit may indicate an incomplete transfer.
		<td>❌
	</tr>
	<tr>
		<td colspan="3">7. Premature Termination
	</tr>
	<tr>
		<td>
		<td>Errors during transfer result in an TFTP_OPCODE_ERROR packet.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Error packets are not retransmitted.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Error packets are not acknowledged.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Timeouts are used to detect errors.
		<td>❌
	</tr>
	<th colspan="3">RFC 2347 - TFTP Option Extension</th>
	<tr>
		<td colspan="3">Abstract
	</tr>
	<tr>
		<td>
		<td>Option negotiation prior to the file transfer support.
		<td>❌
	</tr>
	<tr>
		<td colspan="3">Packet Formats
	</tr>
	<tr>
		<td>
		<td>TFTP options are appended to the Read Request and Write Request packets.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>OACK (Option Acknowledgment, opcode = 6) packet type support.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Error handling for codes 8 (Invalid options.) are implemented.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Sending an TFTP_OPCODE_ERROR packet with error code 8 terminate the transfer.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>TFTP_OPCODE_RRQ and TFTP_OPCODE_WRQ packets containig options have the following format: <li> Opcode (2 bytes) <li> Filename (variable-length string) <li> 0x0 (1 byte) <li> Mode (variable-length string) <li> 0x0 (1 byte) <li> Opt1 <li> 0x0 (1 byte) <li> Value1 <li> 0x0 (1 byte) <li> ... <li> OptN (variable-length string) <li> 0x0 (1 byte) <li> ValueN <li> 0x0 (1 byte)
		<td>❌
	</tr>
	<tr>
		<td>
		<td>OptK is a case-insensitive ASCII sequence terminated by a zero byte.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>ValueK is a case-insensitive ASCII sequence terminated by a zero byte.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>If multiple options are to be negotiated, they are appended to each other.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The maximum size of a request packet is 512 octets.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>OACK packets have the following format: <li> Opcode (2 bytes) <li> Opt1 <li> 0x0 (1 byte) <li> Value1 <li> 0x0 (1 byte) <li> ... <li> OptN (variable-length string) <li> 0x0 (1 byte) <li> ValueN <li> 0x0 (1 byte)
		<td>❌
	</tr>
	<tr>
		<td>
		<td>OptK is a case-insensitive ASCII sequence terminated by a zero byte containing the option acknowledgment copied from the original request.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>ValueK is a case-insensitive ASCII sequence terminated by a zero byte containing the acknowledged value associated with its option.
		<td>❌
	</tr>
	<tr>
		<td colspan="3">Negotiation Protocol
	</tr>
	<tr>
		<td>
		<td>Clients can append options at the end of Read request packet.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Clients can append options at the end of Write request packet.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Any number of options may be specified.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The order of the options is not significant.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>If the server supports option negotiation, and it recognizes one or more of the options specified in the request packet, it may respond with an Options Acknowledgment (OACK).
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Each option the server recognizes, and accepts the value for, is included in the OACK.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The server must not include in the OACK any option which had not been specifically requested by the client.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Options which the server does not support should be omitted from the OACK.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Options which the server does not support should not cause an TFTP_OPCODE_ERROR packet to be generated.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>An option not acknowledged by the server is ignored by the hosts as if it were never requested.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>An OACK packet response to a TFTP_OPCODE_RRQ packet which carries options acknowledges the request and the options it specifies.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A TFTP_OPCODE_DATA packet response to a TFTP_OPCODE_RRQ packet which carries options acknowledges the request but not the options.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>An TFTP_OPCODE_ERROR packet response to a TFTP_OPCODE_RRQ packet which carries options rejects the request.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>An OACK packet response to a TFTP_OPCODE_WRQ packet which carries options acknowledges the request and the options it specifies.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>An TFTP_OPCODE_ACK packet response to a TFTP_OPCODE_WRQ packet which carries options acknowledges the request but not the options.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>An TFTP_OPCODE_ERROR packet response to a TFTP_OPCODE_WRQ packet which carries options rejects the request.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A client attempt to repeat the request without options if it receive an Error packet to a request which carries options
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A client send an TFTP_OPCODE_ACK (with the data block number set to 0) to confirm the values in the OACK packet that acknowledged a TFTP_OPCODE_RRQ packet.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A client begins the transfer with the first TFTP_OPCODE_DATA packet, using the negotiated values in the OACK packet that acknowledged a TFTP_OPCODE_WRQ packet if it accepts them.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A client sends an TFTP_OPCODE_ERROR packet, with error code 8, and terminate the connection in response to the OACK packet if it rejects the values negotiated.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A client acknowledging an OACK, with an appropriate non-error response, use only the options and values returned by the server.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A client receiving an OACK containing an unrequested option respond with an TFTP_OPCODE_ERROR packet, with error code 8, and terminate the transfer.
		<td>❌
	</tr>
	<th colspan="3">RFC 2348 - TFTP Blocksize Option</th>
	<tr>
		<td colspan="3">Abstract
	</tr>
	<tr>
		<td>
		<td>Blocksize negotiation support.
		<td>❌
	</tr>
	<tr>
		<td colspan="3">Blocksize Option Specification
	</tr>
	<tr>
		<td>
		<td>The Blocksize is negotiated if an option contains the ASCII string "blksize" (case in-sensitive).
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The number of octets in a block is specified in ASCII.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Valid values range of the block size are between "8" and "65464" octets, inclusive.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The blocksize refers to the number of data octets (it does not include the four octets of TFTP header).
		<td>❌
	</tr>
	<th colspan="3">RFC 2349 - TFTP Timeout Interval and Transfer Size Options</th>
	<tr>
		<td colspan="3">Abstract
	</tr>
	<tr>
		<td>
		<td>Timeout Interval negotiation support.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Host receiving a file can obtain the total size of the transfer before it begins.
		<td>❌
	</tr>
	<tr>
		<td colspan="3">Timeout Interval Option Specification
	</tr>
	<tr>
		<td>
		<td>The Timeout Interval is negotiated if an option contains the ASCII string "timeout" (case in-sensitive).
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The number of seconds to wait before retransmitting is specified in ASCII.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Valid values of the number of seconds to wait before retransmission range between "1" and "255" seconds, inclusive.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A server willing to accept the timeout option sends an OACK packet having the same timeout specified by the client.
		<td>❌
	</tr>
	<tr>
		<td colspan="3">Transfer Size Option Specification
	</tr>
	<tr>
		<td>
		<td>The Transfer Size is negotiated if an option contains the ASCII string "tsize" (case in-sensitive).
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The size of the file to be transfered is specified in ASCII.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>In Read Request packets, a size of "0" is specified in the request and the size of the file, in octets, is returned in the OACK.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>If the file is too large for the client to handle, it may abort the transfer with an Error packet (error code 3).
		<td>❌
	</tr>
	<tr>
		<td>
		<td>In Write Request packets, the size of the file, in octets, is specified in the request and echoed back in the OACK.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>If the file is too large for the server to handle, it may abort the transfer with an Error packet (error code 3).
		<td>❌
	</tr>
	<th colspan="3">RFC 7440 - TFTP Windowsize Option</th>
	<tr>
		<td colspan="3">Abstract
	</tr>
	<tr>
		<td>
		<td>Window size of consecutive blocks to send negotation support.
		<td>❌
	</tr>
	<tr>
		<td colspan="3">3.  Windowsize Option Specification
	</tr>
	<tr>
		<td>
		<td>The Windowsize is negotiated if an option contains the ASCII string "windowsize" (case in-sensitive).
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The base-10 number of blocks in a window is specified in ASCII.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The valid values range of the number of blocks in a window is between 1 and 65535 blocks, inclusive.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The data sender, after a successful Windowsize negotiation, will transmit the accorded number of consecutive blocks before stopping and waiting for the reception of the acknowledgment of the last block transmitted.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A server willing to accept the windowsize option sends an OACK to the client.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A server windowsize specified value MUST be less than or equal to the value requested by the client.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>A client use the size specified in the OACK or send an TFTP_OPCODE_ERROR packet, with error code 8, to terminate the transfer.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The reception of a data window with a number of blocks less than the negotiated windowsize is the final window.
		<td>❌
	</tr>
	<tr>
		<td colspan="3">4.  Traffic Flow and Error Handling
	</tr>
	<tr>
		<td>
		<td>The data sender cyclically send to the receiver the agreed windowsize consecutive data blocks before normally stopping and waiting for the TFTP_OPCODE_ACK of the transferred window.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The data receiver send to the data sender the TFTP_OPCODE_ACK of the last data block of the window in order to confirm a successful data block window reception.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>In the case of an expected TFTP_OPCODE_ACK not timely reaching the data sender (timeout), the last received TFTP_OPCODE_ACK SHALL set the beginning of the next windowsize data block window to be sent.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>In the case of a data block sequence error, the data receiver SHOULD notify the data sender by sending an TFTP_OPCODE_ACK corresponding to the last data block correctly received.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>The notified data sender SHOULD send a new data block window whose beginning MUST be set based on the TFTP_OPCODE_ACK received out of sequence.
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Traffic with windowsize = 1 MUST be equivalent to traffic specified with request not containing a windowsize option.
		<td>❌
	</tr>
	<tr>
		<td colspan="3">6.  Congestion and Error Control
	</tr>
	<tr>
		<td>
		<td>The rate at which TFTP UDP datagrams are sent SHOULD follow the CC guidelines in Section 3.1 of [RFC5405].
		<td>❌
	</tr>
	<tr>
		<td>
		<td>Implementations SHOULD set a maximum number of retries for datagram retransmissions, imposing an appropriate threshold on error recovery attempts, after which a transfer SHOULD always be aborted to prevent pathological retransmission conditions.
		<td>❌
	</tr>
</table>
