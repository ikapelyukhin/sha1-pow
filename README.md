# sha1-pow

Computes SHA1 proof-of-work: for a given 64 byte prefix finds a suffix such that the SHA1 hash of the resulting string has number of leading zeroes equal to the `difficulty` parameter.

### Building, running and usage

1. `make`
2. `./sha1-pow <prefix> <difficulty>`

### Attributions

Fast SHA1 implementation in assembly is a courtesy of [Project Nayuki](https://www.nayuki.io/page/fast-sha1-hash-implementation-in-x86-assembly).

