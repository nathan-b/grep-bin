# grep-bin
A simple binary grep tool

Does what grep does, but on binary data. No regexp support, just search for the exact token you're looking for.

## Usage

### Search for a string
```
./gb <string> <filename>
```

#### Example
```
./gb ".init" gb
27432   :  61 2e 64 79 6e 00 2e 72 65 6c 61 2e 70 6c 74 00 2e 69 6e 69 74 00 2e 74 65 78 74 00 2e 66 69 6e 69 00 2e 72 6f    | a.dyn..rela.plt..init..text..fini..ro |
27476   :  63 63 5f 65 78 63 65 70 74 5f 74 61 62 6c 65 00 2e 69 6e 69 74 5f 61 72 72 61 79 00 2e 66 69 6e 69 5f 61 72 72    | cc_except_table..init_array..fini_arr |
```

### Search for byte data
```
./gb -b <offset> <hex value> [-b ...] <filename>
```

#### Example
```
./gb -b 0 2e -b 1 72 -b 2 6f gb
   3c460:  6e 69 74 00 2e 74 65 78 74 00 2e 66 69 6e 69 00 2e 72 6f 64 61 74 61 00 2e 65 68 5f 66 72 61 6d 65 5f 68    | nit..text..fini..rodata..eh_frame_h |
   3c4b3:  5f 61 72 72 61 79 00 2e 64 61 74 61 2e 72 65 6c 2e 72 6f 00 2e 64 79 6e 61 6d 69 63 00 2e 67 6f 74 00 2e    | _array..data.rel.ro..dynamic..got.. |
```
