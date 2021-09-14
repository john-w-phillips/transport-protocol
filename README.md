# Transport protocol for 560

## Annotated outputs for lab
Annotated outputs are in
[ABP_Output_Annotated.pdf](/ABP_Output_Annotated.pdf) and
[GBN_Output_Annotated.pdf](/GBN_Output_Annotated.pdf). The design
document is [CSCI 560_Design_Document.pdf](/CSCI 560_Design_Document.pdf).
	
## Building and running
This project uses CMake, you can build and run `abp` as follows:

```sh
$ mkdir -p build
$ cd build
$ cmake ..
$ make
$ ./abp
```

Building the project compiles two executables, `abp` and `gbn`, `abp`
is the lab for 'Alternating Bit Protocol' and gbn is for the 'go back
n' protocol. These executables each run the lab simulation for the appropriate protocol.

The alternating bit protocol is implemented in `proto.c` with some
help from various headers. The go-back-n protocol is implemented in
`GO-BACK-N.c`, again with help from some common headers.


## Misc

I started moving some of the data structures to `transport_common.h`
so unit testing would be possible.
