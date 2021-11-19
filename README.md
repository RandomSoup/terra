# Terra Toolbox
> Yet Another [Piper][1] Toolbox

## What is this?
This project contains a few tools for working with [Piper][1], a new
document-oriented and binary-based network protocol. Due to the "strict"
rules imposed by working with raw binary data with defined characteristics
it's a simpler alternative to other protocols like Gopher, Gemini, or HTTP.

## What is included?
Currently the package includes three tools:
 + Rover: A graphical Piper client with Gemtext support.
 + Ares: A simple proof-of-concept server with PCGI support.
 + PCGI: A simple protocol and library to build CGI-like applications on
 top of Piper.

## Dependencies
To build the package you'll need a GNU-compatible `make` and a C11/17 compiler.
Additionally, to build Rover, you'll have to install the `libX11` development
files from your distribution.

## Building
Simply clone the project and execute `make` in its root directory:
```sh
git clone https://github.com/RandomSoup/terra.git && cd terra
make -j$(nproc)
```
That will place build files under the `build/` directory.

## Limitations
All programs and libraries are prepared to work under Linux systems, but
won't work on other systems like MacOS or Windows, since they make use
of Linux-specific systems call like `epoll`.

Rover doesn't implement raw file downloads yet.

## License and credits
Terra is licensed under a permissive ISC-like license. See [`LICENSE`][2]
for more info.

Rover's design was inspired by [Castor9][3], a Plan 9 Gemini browser, and
uses the [Spleen][4] font, that is licensed under a Simplified BSD License.
The relative URL resolution code was adapted from [`libpiper`][5]'s, with permission.

[1]: https://github.com/Luminoso-256/piper
[2]: https://github.com/RandomSoup/blob/master/LICENSE
[3]: https://sr.ht/~julienxx/Castor9/
[4]: https://github.com/fcambus/spleen
[5]: https://github.com/RandomSoup/libpiper
