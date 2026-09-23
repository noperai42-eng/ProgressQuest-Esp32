# vendor/

Third-party trees. `pq-cli` is **reference only**. `stb_easy_font.h` and `stb_image.h` are linked by `host/gfx.c`.

## pq-cli

Faithful Progress Quest logic, cloned from https://github.com/rr-/pq-cli
(MIT, Copyright (c) 2018 Marcin Kurczewski). That project ports
https://bitbucket.org/grumdrig/pq, Eric Fredricksen's source release.
The original game is MIT: http://progressquest.com/license.txt
(Copyright (c) 2002–2004; version 6.4's `dist/license.txt` is Copyright (c) 2022).

Use it for ideas (task state machine, encumbrance → market, spell ranks,
11 armor slots, plot acts). Do **not** copy word lists, joke races, or spell
names into `app/`. Tami’s names stay original folklore. This checkout is
gitignored and is not part of the public repository.

Original game: http://progressquest.com/
