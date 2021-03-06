# zatch

Do stuff based on changes in your local file system. I.e. sync some files to a
server as soon as something changes locally, or repeatedly restart a process
while you're writing code.

Features:
* Easy to integrate in shell scripts, zatch simply echoes the name of a dir with
  changes to stdout
* Fast and lightweight (uses the native FSEvents API of macOS)
* Small and no third-party dependencies

Status: **stable**


## Examples

Run rsync anytime a file in *foo* or *bar* or any of it's subdirectories
changes:

```sh
zatch foo bar | while read _path; do rsync -az "$_path" example.com: ; done
```

Show a macOS notification with the name of the subdirectory that contains
changes, but filter out any component named *xar*:

```sh
zatch -s foo bar | fgrep --line-buffered -v '/xar/' | while read _path
	do osascript -e "display notification \"$_path\""
done
```

Initially start Node.js, and restart whenever a file changes in the current
working dir or any of it's subdirectories:

```sh
node server.js & zatch . | while read -r p; do echo "$p" && kill $! ; node server.js & done
```


### Requirements

* macOS


## Installation

Compile and install zatch:

```sh
$ git clone https://github.com/timkuijsten/zatch.git
$ cd zatch
$ make
$ sudo make install
```


## Documentation

For documentation please refer to the manpage [zatch(1)].


## License

ISC

Copyright (c) 2017, 2018, 2019 Tim Kuijsten

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.

[zatch(1)]: https://netsend.nl/zatch/zatch.1.html
