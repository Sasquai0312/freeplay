# 07 — ARM Thumb Compatibility

## Toolchain

`clang -target thumbv7em-none-eabihf -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -Os -ffreestanding -fno-builtin`

Same family as FreePlay CFW builds.

## Result

All eight engine `.c` files compile to Thumb objects successfully. No libc calls in engine modules. Relocatable whole-archive link skipped (no bare-metal `-lc` in this image); per-module `llvm-size` used instead.

## Flags for future firmware link

`-ffunction-sections -fdata-sections` + `--gc-sections` so unused generators drop out.
