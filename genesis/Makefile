# Genesis portable engine — host tests, demos, Thumb size report
ROOT := $(abspath ..)
CC ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -O2 -Iinclude
SAN ?=
ENGINE_SRCS := engine/event.c engine/random.c engine/scales.c engine/rhythm.c \
               engine/melody.c engine/chords.c engine/arpeggiator.c engine/transform.c
ENGINE_OBJS := $(ENGINE_SRCS:.c=.o)

.PHONY: all test demos clean thumb-size host-asan zip-demos

all: tests/test_genesis tools/gen_demos

engine/%.o: engine/%.c include/genesis.h
	$(CC) $(CFLAGS) $(SAN) -c $< -o $@

tests/test_genesis: tests/test_genesis.c tools/smf_export.c $(ENGINE_OBJS) include/genesis.h include/genesis_smf.h
	$(CC) $(CFLAGS) $(SAN) -Iinclude -o $@ tests/test_genesis.c tools/smf_export.c $(ENGINE_OBJS)

tools/gen_demos: tools/gen_demos.c tools/smf_export.c $(ENGINE_OBJS)
	$(CC) $(CFLAGS) -Iinclude -o $@ tools/gen_demos.c tools/smf_export.c $(ENGINE_OBJS)

test: tests/test_genesis
	./tests/test_genesis

demos: tools/gen_demos
	./tools/gen_demos examples

zip-demos: demos
	cd examples && zip -q -r ../genesis_demos.zip *.mid *.txt && mv ../genesis_demos.zip .
	@echo "wrote examples/genesis_demos.zip"

host-asan:
	$(MAKE) clean
	$(MAKE) SAN='-fsanitize=address,undefined -fno-omit-frame-pointer' test

# ARM Thumb freestanding size (no link against firmware)
THUMB_CC ?= clang
THUMB_CFLAGS = -std=c11 -Os -ffunction-sections -fdata-sections \
	-target thumbv7em-none-eabihf -mcpu=cortex-m7 -mthumb -mfloat-abi=hard \
	-ffreestanding -fno-builtin -Iinclude -DNDEBUG

thumb-size:
	@mkdir -p build/thumb
	@echo "=== Thumb object sizes (-Os) ===" | tee build/thumb/sizes.txt
	@total=0; for s in $(ENGINE_SRCS); do \
	  base=$$(basename $$s .c); \
	  $(THUMB_CC) $(THUMB_CFLAGS) -c $$s -o build/thumb/$$base.o; \
	  sz=$$(arm-none-eabi-size -A build/thumb/$$base.o 2>/dev/null | awk '/^\.text/ {t+=$$2} /^\.rodata/ {t+=$$2} /^\.data/ {t+=$$2} END {print t+0}'); \
	  if [ -z "$$sz" ] || [ "$$sz" = "0" ]; then \
	    sz=$$(stat -c%s build/thumb/$$base.o); \
	    echo "$$base.o: object_file_bytes=$$sz (size tool unavailable; file size)" | tee -a build/thumb/sizes.txt; \
	  else \
	    echo "$$base: text+rodata+data=$$sz" | tee -a build/thumb/sizes.txt; \
	  fi; \
	  total=$$((total+sz)); \
	done; \
	echo "SUM_MODULE_BYTES=$$total" | tee -a build/thumb/sizes.txt
	@llvm-size -A build/thumb/*.o 2>/dev/null | tee -a build/thumb/sizes.txt || true
	@python3 -c "import subprocess,glob;t=0\
;\
\
for p in sorted(glob.glob('build/thumb/*.o')):\
  o=subprocess.check_output(['llvm-size','-A',p],text=True);s=0\
;\
  \
  [s:=s+int(l.split()[1]) for l in o.splitlines() if l.split() and (l.split()[0].startswith('.text') or l.split()[0].startswith('.rodata') or l.split()[0]=='.data')];\
  print(p.split('/')[-1],s);t+=s\
;\
print('TOTAL_TEXT_RODATA',t)" | tee -a build/thumb/sizes.txt

clean:
	rm -f $(ENGINE_OBJS) tests/test_genesis tools/gen_demos
	rm -rf build/thumb
