# zatch

Efficiently watch directory changes on macOS.

Features:
* Simply echos the name of a dir with changes on stdout
* Uses the FSEvents API of macOS

Status: **beta**

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


### Example

rsync all files in the directories *foo* and *bar* recursively to example.com
anytime something in it or in any of it's subdirectories changes:
```sh
zatch foo bar | while read _path; do rsync -az "$_path" example.com: ; done
```


## Documentation

For documentation please refer to the manual.


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
