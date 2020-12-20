# JXServer

A networked server that sends ﬁles to clients in response to requests, which will support multiple connecting clients simultaneously as well as multiple simultaneous connections from the same client for increased transfer speeds.

## Introduction

Typically, networking functionality is implemented by kernel space code. On Linux and other Unixlike operating systems, the kernel implements the various components required for networking, such as Wiﬁ capabilities and the Internet Protocol. This is made available to userspace programs through special system calls, which interact with an abstraction known as a socket.

Sockets represent a connection to another endpoint on a network, but are analogous to ﬁle handles and are also described by a ﬁle descriptor. "Reading" from or "writing" to a socket corresponds to receiving or sending data over the network. However, as there are specialised operations that need to be performed with sockets, such as accepting or making connections to network destinations, there are special system calls that are generally used.

This server will use standard Transmission Control Protocol (TCP) connections on top of the Internet Protocol, version 4 (IPv4). This is a standard networking protocol combination for ubiquitous services such as HTTP which we use on the "World Wide Web".

The combination of TCP and IP(v4), as implemented and exposed to userspace by the kernel, permits the formation of network connections. A client initiates a connection to a server. Both client and server endpoints are deﬁned by an IP address, which in IPv4 is a 32 bit unsigned integer, as well as a TCP port number, which is a 16 bit unsigned integer. Once the server accepts the connection, the endpoints exchange data to setup a reliable connection. Data that userspace programs send into the socket, at either end of the connection, is delivered and made available for receiving by the userspace application at the other endpoint. TCP guarantees that data is delivered reliably and in order over varying network conditions. It also permits either endpoint to send data simultaneously to each other. However, it treats data as a continuous stream rather than discrete messages; that is for example if an endpoint sends 5 bytes then 10 bytes, the other endpoint will be able to read the data as it is received, with no indication that there was originally a break in sending 5 bytes in.

## Usage 

### Generate Configure 

```
Usage: ./generate-config <dotted quad ipv4 address xxx.xxx.xxx.xxx> <port number> <target directory>
For example: ./generate-config 127.0.0.1 8888 /this/is/my/directory
```

### Build Server 

``` 
make all
```

### Build Client

``` 
make client
```

### Clean the Program

```
make clean
```

## Functionality

This server will listen for and accept connections from clients. Upon a successful connection, clients will send a number of possible requests, described in the below format. Please note that all integer ﬁelds are sent in network byte order. This server should not send any data except in response to a client request.

When a client is ﬁnished making requests on a given connection, they will shutdown the connection. When you detect this, the server should close that socket and clean up any associated data in your program.

All client requests and server responses consist of one or more structured series of bytes, called messages. Each message will contain the following ﬁelds in the below format:

- 1 byte - Message header; this describes details about the message as follows.
  - First 4 bits - "Type digit": A single hexadecimal digit that deﬁnes the type of request or response. It is unique for different types of messages.
  - 5th bit - "Compression bit": If this bit is 1, it indicates that the payload is compressed. Otherwise, the payload is to be read as is.
  - 6th bit - "Requires compression bit": If this bit is 1 in a request message, it indicates that all server responses (except for error responses) must be compressed. If it is 0, server response compression is optional. It has no meaning in a response message.
  - 7th and 8th bits - padding bits, which should all be 0.

- 8 bytes - Payload length; unsigned integer representing the length of the variable payload in bytes (in network byte order).
- Variable byte payload - a sequence of bytes, with length equal to the length ﬁeld described previously. It will have different meanings depending on the type of request/response message.

The full contents of an example message, as a hexdump, are shown below. Note that it is a valid example of a message in this network protocol, but does not correspond to any meaningful request or response.

``` 
d8 00 00 00 00 00 00 00 07 ab ab ab ab ab ab ab
```

This message is explained in order below:

- d8 - 1 byte message header. This is represented by the binary 11011000.
  - 1101 - 4 bits representing the hexadecimal type 0xD
  - 1 - 1 bit ﬂag indicating this payload is compressed
  - 0 - 1 bit ﬂag which means, if this message is a request, the response does not have to be compressed. If this message is a response, it has no meaning.
  - 00 - 2 bits of 0 padding

- 00 00 00 00 00 00 00 07 - 8 bytes representing the payload length 7 in network byte order
- ab ab ab ab ab ab ab - the 7 byte payload

On any single connection, clients will only send one request at a time, before waiting for the appropriate response. That is, after sending a request, the client will wait for the server to send a complete response before sending the next request. For some requests, the server will need to send multiple response messages. An example is ﬁle retrieval, where the server may need to send a ﬁle to the client split over many response messages. In this case, the client will wait until all appropriate response messages are received before sending the next request, if any.

### Error functionality

If server receive a client request with invalid (unknown) type ﬁeld, this server is to send back a response with type digit 0xf, with no payload (payload length ﬁeld 0), and then close the connection. Server should also send this error message if there are any other errors that arise in requests with valid request type ﬁelds. Server are to also send back this error response if server receive a client request with a type ﬁeld that must only be used in server response messages. Error messages are never compressed, even if the original request indicated compression is required.

### Echo functionality

Clients may request an "echo". The request type digit will be 0x0. There will be an arbitrary sequence of bytes in the payload of the request.

In response to this request, your server is expected to send back a response with type 0x1. The payload of your response should contain the same payload server received in the request. Note that if the original request requires compression, then you need to compress the payload before returning it in the response. However, after decompression, the payload should be identical to the one you received.

### Directory listing

The request type will be 0x2. There will be no payload and the payload length ﬁeld will be 0. The client is requesting a list of all ﬁles in the server's target directory.

In response to this request, server is expected to send back a response with type 0x3. The payload of your response should contain the ﬁlenames of every regular ﬁle in the target directory provided in the command line arguments to server. The ﬁlenames can be returned in an arbitrary order.

These ﬁlenames are to be sent end to end in the payload, separated by NULL (0x00) bytes. Include a NULL (0x00) byte at the end of the payload. Server will need to set the payload length appropriately. If the directory is empty, send a single NULL (0x00) byte as a payload.

### File size query

The request type will be 0x4. The request payload will be a NULL terminated string that represents a target ﬁlename, for which the client is requesting the size.

In response to this request, your server is expected to send back a response with type 0x5. The payload of your response should contain the length of the ﬁle with the target ﬁlename in your target directory, in bytes, represented as a 8-byte unsigned integer in network byte order. If the requested ﬁlename does not exist, return an error response message.

### Retrieve ﬁle

The request type will be 0x6. This is a request for part or whole of a ﬁle in your server's target directory. The payload will consist of the following structure:

- 4 bytes - an arbitrary sequence of bytes that represents a session ID for this request. 

- 8 bytes - the starting offset of data, in bytes, that should be retrieved from the ﬁle

- 8 bytes - the length of data, in bytes, that should be retrieved from the ﬁle

- Variable bytes - a NULL terminated string that represents a target ﬁlename

The ﬁlename will not contain any relative or absolute path components, you only need to search in the target directory, and no subdirectories.

In response to this request, your server is expected to send back one or more response messages with type 0x7. Each response of type 0x7 may represent a portion of the requested ﬁle data. It is up to you how many and how large these portions you send are. It is also up to you the order in which you send these portions; the start of the ﬁle does not need to be sent ﬁrst, as long as all requested data is eventually received by the client. Different portions which you send corresponding to the same original request must not overlap in the byte ranges from the target ﬁle they contain. Each payload must consist of the following structure:

- 4 bytes - the same session ID as was provided in the original request

- 8 bytes - a starting offset of data, in bytes, from the target ﬁle, that this response contains

- 8 bytes - the length of data, in bytes, from the target ﬁle, that this response contains

- Variable bytes - the actual data from the target ﬁle at the declared offset and length in this response

The client may open several concurrent requests for the same ﬁlename on different simultaneous connections, with the same session ID. If you receive multiple connections with requests for the same ﬁle range with the same session ID, it means you are able to multiplex your ﬁle data across those connections; a single requesting client is unifying the data at the other end. If you choose to do this, you need to ensure that across all connections sharing the same session ID, the whole requested range of the ﬁle is eventually reconstructed. The client may make an extra concurrent connection for a given ﬁle at any time.

In the diagram below, the blue double headed arrows indicate a successful connection. Originally, the client has opened one connection, requesting the ﬁle target.txt, with session ID 0x67A5CC30. The client then opens a second connection, requesting the same ﬁle. Because the session ID is the same, the server accepts the connection, and is able to return ﬁle data simultaneously over the two connections (note that the requested ﬁle range must also be the same, but this is not shown in the diagram).

<img src='https://i.loli.net/2020/12/20/F2rnOmE6cBgs9a1.png' alt='F2rnOmE6cBgs9a1'/>

You do not have to multiplex your ﬁle response across multiple connections. If so, for connections on which you will not be returning data, you can send a response with type 0x7 with empty payload. However, your program must be returning the requested response on at least one connection among those the client opens.

If you would like to achieve higher performance, you will need to implement multiplexing of your ﬁle response across multiple connections.

It is not valid to receive a request for a different ﬁle, or the same ﬁle with a different byte range, with the same session ID as a currently ongoing transfer. If this occurs, you should send an error response message However, once the entirety of a ﬁle is transferred, the session ID may be reused for different ﬁles or the same ﬁle with a different byte range, in subsequent requests.

In the below example diagram, the red arrow indicates a failed connection where an error response should be sent. The client has an existing connection requesting the ﬁle target.txt, with session ID 0x67A5CC30. It has attempted to open a new connection requesting the different ﬁle otherﬁle.txt, with the same session ID. This is invalid; however the client is able to make a request for otherﬁle.txt, shown using the different session ID 0x1200CFBA. The server is then expected to service these two requests simultaneously.

<img src='https://i.loli.net/2020/12/20/nw5tqkOQ28IJrvV.png' alt='nw5tqkOQ28IJrvV'/>

You may receive a request for the same ﬁle with a different session ID while that ﬁle is being transferred under a ﬁrst session ID. This is considered a separate client that requires a separate copy of the ﬁle and should receive the appropriate response.

If you receive any other invalid request, such as the ﬁlename not existing, or the requested byte range being invalid for the size of the target ﬁle, you must send an error response message.

### Shutdown command

The request type digit will be 0x8 and the payload will be 0 length. Your server does not send any response. Instead, your server will shutdown and close all current connections, and exit, cleaning up/freeing all threads/processes/memory, as required. You are guaranteed there will be no further new connections or requests after this command. You are guaranteed a Shutdown command will only be sent after your server completes processing all other requests. After all processing has been completed and resources freed, your server should terminate with exit code 0.

### Lossless compression

For any message where the compression bit is set in the message header, the variable length payload is losslessly compressed, which means it is encoded in a way that completely retains the original payload information, but aims to reduce the size by applying a compression algorithm to the data. The payload will have the following structure in order (note that the sections are not necessarily aligned to bytes):

- Variable number of bits - Compressed payload. See below for details. It is not necessarily aligned to a byte boundary.

- Variable number of bits - padding with 0 bits to the next byte boundary. This ensures that the structure is aligned to a byte boundary.

- 1 byte - an unsigned integer representing how many bits of 0 padding were required in the previous ﬁeld

Note that the payload length ﬁeld of the compressed message will contain the length of the compressed payload in bytes. Note that the padding in the compressed payload ensures it is aligned to whole bytes in size.

Your server may also receive request messages (which may also be compressed) where the "Requires compression" bit is set in the message header. This means that any message(s) that your server sends in response to such messages must be compressed. Your server should never set this bit in a response message; it is only valid in request messages. If this bit is not set for a request, then it is up to you whether to compress response(s) to that request. Compression can reduce the amount of data needing to be sent over the network, but requires a processing time tradeoff to compute the compressed payload.

Server will apply compression by replacing bytes of uncompressed data with variable length bit sequences ("bit codes"). This works by aiming to encode more frequently used bytes to shorter bit codes, and less frequently used bytes to longer codes. Therefore, it is possible to have data which does not compress at all, or in fact "compresses" to a larger size. The compression dictionary deﬁnes the mapping of bytes to bit codes and consists of 256 segments of variable length. Each segment corresponds in order to input byte values from 0x00 to 0xFF. Each segment is not necessarily aligned to a byte boundary. However, at the end of the 256 segments, there is padding with 0 bits to the next byte boundary. This means the entire compression dictionary is aligned to a byte boundary overall. The overall structure follows:

- 256 of the following segments, with no padding in between (including no padding to byte boundaries):
  - 1 byte - unsigned integer representing the length of this code in bits; this is equal to the length of the next ﬁeld in bits. It is not necessarily aligned to a byte boundary.
  - Variable number of bits - the bit code that is used to encode the byte value corresponding to this segment. It is not necessarily aligned to a byte boundary.

- Variable number of bits - padding with 0 bits to the next byte boundary. This ensures that the entire dictionary is aligned to a byte boundary.

You will be provided with a binary ﬁle that contains a compression dictionary in the structure described above. This binary ﬁle will always be called compression.dict and be present in the same directory as your server executable during testing. The compression dictionary stays the same for all requests handled each time your server runs. That is, your server only needs to read the ﬁle once at start up to obtain the dictionary, then use that dictionary to handle all compressed requests or responses.

An example of the start of a compression dictionary is shown below. Note that this only includes a few segments and the full dictionary would include all 256 segments plus 0 bit padding at the end as required:

``` 
04 c0 4f ...
```

This data is explained in order below:

<img src='https://i.loli.net/2020/12/20/7KiLXOeSTcsb1o9.png' alt='7KiLXOeSTcsb1o9'/>

- The ﬁrst segment corresponds to the byte 0x00

- 04 - the ﬁrst byte is the length of the code for 0x00 in bits (i.e. 4 bits)

- c0 - the binary for this byte is 11000000. The ﬁrst 4 bits, 1100, is therefore the bit code for 0x00. The next 4 bits is the start of the second segment, corresponding to the byte 0x01. The size of the entire ﬁrst segment for byte 0x00 is 12 bits.

- 4f - the binary for this byte is 01001111. Remember that segments are not necessarily aligned to byte boundaries. Therefore, the byte representing the length of the code for 0x01 is comprised of the second 4 bits from c0 and the ﬁrst 4 bits from 4f. The binary is therefore 00000100, which corresponds to a code length of 4 bits as well. This means that the second 4 bits of 4f, 1111, is the code for 0x01.
- The size of the second segment is also 12 bits. Note that segments will vary in size. Depending on the size of each segment, the boundary of each segment can be at any bit offset within a byte, not necessarily 0 or 4 bits like in this short example.

To create the compressed payload, for each byte in the original payload, obtain the segment in the compression dictionary corresponding to the byte value. In your compressed output, the bit code for this segment should be output for this input byte.

Using just the segments shown in the example, you could compress data containing bytes 0x00 and 0x01. For the following example uncompressed data:

``` 
01 00 00 01 01 00
```

Simply replace each uncompressed byte with the bit code from the compression dictionary. That is, the compressed binary would be:

``` 
1111 1100 1100 1111 1111 1100
```

This would result in the ﬁnal compressed bytes:

``` 
fc cf fc
```

The compressed payload is output as is. However, at the end of the compressed payload, there is 0-bit padding to the nearest byte boundary, and a single byte indicating how much padding there was (see above).

To decode compressed data, as you read in bits from the compressed payload, simply reverse the process by using the compression dictionary to ﬁnd the original byte values. When you decompress a payload, interpret that payload as per the other functionality of your server, depending on the message type digit contained in the message header.

For the "Requires compression" bit, you do not ever compress already compressed data. For example, if you receive an echo request with compressed payload, and "Requires compression" set, you do not compress the payload again in your response.

As you may note, input bytes are encoded to variable length bit codes ("variable length coding"). It is guaranteed that your compression dictionary gives you bit codes that are uniquely decodable. This means there is only one way to decode a compressed payload, even though you are not explicitly informed where the variable length bit codes start and end.

## Notes

Remember to comment or uncomment in `common.h` file.

<img src='https://i.loli.net/2020/12/20/ThsQWcE214ynYbN.png' alt='ThsQWcE214ynYbN'/>