# Deprecated - see [Straw](https://github.com/aidenlab/straw)

## JuiceboxReader
Fast C++ implementation of reading/dump for .hic files. 

Not as fully featured as the Java version. Reads the .hic file, finds 
the appropriate matrix and slice of data, and outputs as text in sparse 
upper triangular format.

Currently only supporting matrices.

Must be compiled with the -lz flag to include the zlib.h library:
`g++ -lz -o  juicebox-quick-dump dump.cc`

Usage: `juicebox-quick-dump <NONE/VC/VC_SQRT/KR> <hicFile(s)> <chr1>[:x1:x2] <chr2>[:y1:y2] <BP/FRAG> <binsize> `
