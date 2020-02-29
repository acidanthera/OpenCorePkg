GOP Testing Tool
================

1. Disable watchdog timer.
2. Gather all N available GOP protocols.
3. Allocate N buffer for GOP reports (`EFI_PAGE_SIZE`).
4. Get GOP reports for every GOP and set maximum resolution.
5. Save GOP reports to file to any writable fs (`gop-{date}.txt`)
6. Run tests in every GOP with non-zero resolution.

### GOP Report

```
GOP #(%u+1) of total %u, console/auxiliary
Current mode %u, max mode %u, FB: %p, FBS: %X
Current: W x H, pixel %u (%X %X %X %X), scan: %u
0: W x H, pixel %u (%X %X %X %X), scan: %u
1: Invalid parameter
New mode %u, max mode %u, FB: %p, FBS: %X
New: W x H, pixel %u (%X %X %X %X), scan: %u
```

### GOP Test

1. Fill screen with Red (#FF0000) in direct mode.
1. Wait 5 seconds.
1. Fill screen with 4 rectangles. From left to right top to bottom they should contain the following colours: Red (#FF0000) / Green (#00FF00) / Blue (#0000FF) / White (#FFFFFF). The user should visually ensure that the colours match and that rectangles equally split the screen in 4 parts.
1. Wait 5 seconds.
1. **UNIMPLEMENTED** Fill screen with white text on black screen in normal mode. The screen should effectively be filled with 9 equal rectangles. From left to right top to bottom they should contain the following symbols: A B C D # F G H I. Instead of # there should be current GOP number. The user should visually ensure text and background colour, rectangle sizes, rectangle data.
1. **UNIMPLEMENTED** Wait 5 seconds.
1. **UNIMPLEMENTED** Fill screen with white text on black screen in HiDPI mode. This should repeat the previous test but the text should be twice bigger. The user should ensure all previous requirements and the fact that the text got bigger.
1. **UNIMPLEMENTED** Wait 5 seconds.
1. **UNIMPLEMENTED** Print all GOP reports one by one (on separate screens) with white text in black screen in normal mode. Wait 5 seconds after each. The user should screenshot these and later compare to the file if available.
1. Fill screen with 9 rectangles of different colours. From left to right top to bottom they should contain the following colours: Red (#FF0000), Green (#00FF00), Blue (#0000FF), White (#FFFFFF), Light Grey (#989898), Black (#000000), Cyan (#00FFFF), Magenta (#FF00FF), Yellow (#FFFF00). visually ensure that the colours match and that rectangles equally split the screen in 9 parts.
1. Wait 5 seconds.

### Hints

- Due to not all resolutions being divisible by 2 and 3, right and bottom pixel rows may be unused in corresponding tests.
- Depending on rendering performance and amount of GOPs the test may take up to several minutes.
- Reference images for some tests are availale in `Examples` directory.

