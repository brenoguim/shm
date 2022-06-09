This repository provides:
- Implementation of IPC from parent to child process using shared memory
  - IPC using pipes could be easily replaced by this solution
- Extension of `std::streambuf` to work with shared memory
- Implementation of a `write` and `read` processes demonstrating how the communication can happen and the speed measurements.

```
$ ./write --manual --iostream --block-size 32 --gbs 2

[write] Using manual writing
Start reading [disable memcpy = 0]
Read 2 GB in 0.329 seconds | 6.07903 GB/s
    Exactly: 2147483648 bytes

[write] Using iostreams
Start reading [disable memcpy = 0]
Read 2 GB in 1.718 seconds | 1.16414 GB/s
    Exactly: 2147483648 bytes
```
