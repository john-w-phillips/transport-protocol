# Transport protocol for 560
## Annotated outputs for lab
Annotated outputs are in
[ABP_Output_Annotated.pdf](/ABP_Output_Annotated.pdf) and
[GBN_Output_Annotated.pdf](/GBN_Output_Annotated.pdf).
	
## Building and running
This project uses CMake:

```sh
$ mkdir -p build
$ cd build
$ cmake ..
$ make
$ ./abp
```

Building the project compiles two executables, `abp` and `gbn`, `abp`
is the lab for 'Alternating Bit Protocol' and gbn is for the 'go back
n' protocol.


## Misc

I started moving some of the data structures to `transport_common.h`
so unit testing would be possible.
