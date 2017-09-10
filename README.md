# zatch

Do stuff based on changes in your local file system. I.e. sync some files to a
remote as soon as something changes locally, or repeatedly restart a process
while you're writing code.

Features:
* Easy to integrate in shell scripts, zatch simply echoes the name of a dir with
  changes to stdout
* Fast without taking a lot of resources (uses the FSEvents API of macOS)
* Small and no runtime dependencies

Status: **beta**

zatch is primarily developed and tested on macOS 10.12.


## Example

rsync all files in the directories *foo* and *bar* recursively to example.com
anytime something in it or in any of it's subdirectories changes:
```sh
$ zatch foo bar | while read _path; do rsync -az "$_path" example.com: ; done
```


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


## Documentation

For documentation please refer to the manual (`$ man zatch`).


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
