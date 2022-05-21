# 90spot
Spotify player for Nintendo Switch

## Prerequisites

You will need to install SDL2, SDL2_ttf, libogg, and libvorbis portlibs to your devkitpro install:
```
sudo (dkp-)pacman -S switch-sdl2 switch-sdl2_ttf switch-libogg switch-libvorbis
```

## Licenses
We use the following libraries:
 * [cJSON](https://github.com/DaveGamble/cJSON) by [Dave Gamble](https://github.com/DaveGamble) and [cJSON contributors](https://github.com/DaveGamble/cJSON/graphs/contributors). Licensed under [MIT](https://github.com/DaveGamble/cJSON/blob/master/LICENSE).
 * [protobuf-c](https://github.com/protobuf-c/protobuf-c) by Dave Benson and [the protobuf-c authors](https://github.com/protobuf-c/protobuf-c/graphs/contributors). Licensed under [BSD-2.0](https://github.com/protobuf-c/protobuf-c/blob/master/LICENSE).
 * [gmp-mini](https://gmplib.org/) by [GMP contributors](https://gmplib.org/manual/Contributors). Licensed under GPL-2.0 or newer.
 * [shannon](https://web.archive.org/web/20080719073929/http://www.qualcomm.com.au/PublicationsDocs/Shannon-1.0.tgz) by Qualcomm International. Licensed under Apache-1.1.
 * [libogg](https://github.com/xiph/ogg) (via static link + headers) by the [Xiph.org Foundation](https://xiph.org/). Licensed under [BSD-2.0](https://github.com/xiph/ogg/blob/master/COPYING).
 * [libvorbis](https://github.com/xiph/vorbis) (via static link + headers) by the [Xiph.org Foundation](https://xiph.org/). Licensed under [BSD-2.0](https://github.com/xiph/vorbis/blob/master/COPYING).
 * [SDL2](https://github.com/libsdl-org/SDL) (via static link + headers) by [Sam Lantinga](https://github.com/slouken) and the [SDL contributors](https://github.com/libsdl-org/SDL/graphs/contributors) **AND** the [Switch fork](https://github.com/devkitPro/SDL/tree/switch-sdl2-2.0.14-dev) made by devkitPro & various contributors. Licensed under zLib.
 * [SDL_ttf](https://github.com/libsdl-org/SDL_ttf) (via static link + headers) by [Sam Lantinga](https://github.com/slouken) and the [SDL contributors](https://github.com/libsdl-org/SDL/graphs/contributors). Licensed under zLib.
    * [freetype](https://gitlab.freedesktop.org/freetype/freetype) (via static link) by David Turner, Robert Wilhelm, Werner Lemberg, and [freetype contributors](https://gitlab.freedesktop.org/freetype/freetype/-/graphs/master). Licensed under [The FreeType Project LICENSE](https://gitlab.freedesktop.org/freetype/freetype/-/blob/master/docs/FTL.TXT).
 * [SDL2_gfx](https://www.ferzkopp.net/wordpress/2016/01/02/sdl_gfx-sdl2_gfx/) (stripped down) by [A. Schiffler / ferzkopp](https://www.ferzkopp.net/). Licensed under zLib.

Protobuf files found in `source/proto` are compiled with `protoc-c` of the [protobuf-c](https://github.com/protobuf-c/protobuf-c) project by Dave Benson and [the protobuf-c authors](https://github.com/protobuf-c/protobuf-c/graphs/contributors), licensed under [BSD-2.0](https://github.com/protobuf-c/protobuf-c/blob/master/LICENSE). The protobuf files originate from the [librespot](https://github.com/librespot-org/librespot) project by [Paul Lietar](https://github.com/plietar) and [librespot contributors](https://github.com/librespot-org/librespot/graphs/contributors), licensed under [MIT](https://github.com/librespot-org/librespot/blob/dev/LICENSE).

90spot's Spotify interfacing code is heavily based on the [librespot](https://github.com/librespot-org/librespot) project by [Paul Lietar](https://github.com/plietar) and [librespot contributors](https://github.com/librespot-org/librespot/graphs/contributors), licensed under [MIT](https://github.com/librespot-org/librespot/blob/dev/LICENSE).

We also use the [Inter](https://rsms.me/inter/) font, made by [Rasmus Andersson](https://rsms.me/), licensed under [SIL Open Font License 1.1](https://github.com/rsms/inter/blob/master/LICENSE.txt).
