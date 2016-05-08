dCa
===

The only [Discord Audio](https://github.com/bwmarrin/dca/wiki/DCA1-specification) encoder you'll ever need (probably).

It's written in C, it should be pretty fast, it wraps ffmpeg, and it can be used as a library.
You could probably write bindings to other languages too!

Building
--------

You need [CMake](https://cmake.org) v3.1 or later.

```
cmake .
make
./bin/dca
```

Feaures
-------

- [x] Encoding
- [ ] DCA 0
- [ ] DCA 1
    - [ ] Magic Bytes
    - [ ] Tool Header
    - [ ] OPUS Header
    - [ ] Origin Header
    - [ ] Info Header
    - [ ] Extra Headers
- [ ] File Inspection
- [ ] Decoding
