# Protocol

Messages are newline-delimited JSON objects.

Message types:

- `proposal`
- `decision`
- `status`

All messages include:

- `v`: protocol version
- `t`: message type
- `seq`: sequence number
- `crc32`: CRC-32 over canonical JSON without the `crc32` field

Maximum sizes are enforced in firmware and on the host.
