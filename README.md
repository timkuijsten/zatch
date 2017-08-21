# zatch

Efficiently watch directory changes on macOS.

Features:
* Simply echos the name of a dir with changes on stdout
* Uses the FSEvents API of macOS

Status: **alpha**

zatch is primarily developed and tested on macOS 10.12.


## Installation

Compile and install zatch:

```sh
$ git clone https://github.com/timkuijsten/zatch.git
$ cd zatch
$ make
$ sudo make install
```


### Build requirements

* C compiler


### Examples

rsync all files in foo to example.com after some of it's descendants change:
```sh
zatch foo | awk '{ system("rsync -az foo example.com:") }'
```


## Documentation

For documentation please refer to the manual.


## Wishlist

* support -s option to echo subdirs


## License

ISC

Copyright (c) 2017 Tim Kuijsten

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
