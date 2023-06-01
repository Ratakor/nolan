# Nolan
A discord bot for Orna.

# Dependencies:
- [concord](https://github.com/Cogmasters/concord)
- [tesseract](https://github.com/tesseract-ocr/tesseract)
- [gd](https://github.com/libgd/libgd)

## TODO:

- separate nolan.h in a raids and stats header
- for on_interactions, do the content testing in on_... instead to throw an
  error when needed
- update cmd_help
- refactor cropping in ocr.c
- refactor parseline() in stats.c
- add player with name in event->content for ham
- add option to correct stats

- automatic roles attribution (for Orna FR) -> with updatemsg
- ascensions (track mats)
- better wrong image input detection

- bug with updatemsg ?
