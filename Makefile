# Firmware for the Atech modular boards. One build = an app from src/ plus a board header.
#
#   make build APP=pocket-synth      build one app          (default APP: pocket-synth)
#   make flash APP=grid-selftest     build and flash it
#   make monitor                     serial console
#   make list                        the builds this repo defines
#   make dist APP=pocket-synth       collect bootloader, partition table and app into dist/
#   make sdk / make sync-sdk         install the open Atech SDK and re-copy the real drivers
#   make board-headers               re-render lib/atech_board/variants/ from boards/*.yaml
#   make check / make send KEY=.. VALUE=..   talk to a connected board through the SDK
#   make grid-view                   live 8x8 view of the distance sensor (sid-theremin build)
#   make keys                        play the sid-theremin build from this keyboard, over Bluetooth MIDI
#
# Needs: platformio (`pio`), uv. PORT defaults to the first ESP32-S3 CDC device.

APP   ?= pocket-synth
PORT  ?= $(firstword $(wildcard /dev/cu.usbmodem*) /dev/cu.usbmodem101)
ATECH  = .venv/bin/atech

.PHONY: build flash monitor list dist clean sdk sync-sdk check send idf-minimal board-headers grid-view keys

build:
	pio run -e $(APP)

flash: build
	pio run -e $(APP) -t upload --upload-port $(PORT)

monitor:
	pio device monitor -p $(PORT) -b 115200

list:
	@sed -n 's/^\[env:\(.*\)\]/  \1/p' platformio.ini

dist: build
	@mkdir -p dist/$(APP)
	@cp .pio/build/$(APP)/bootloader.bin dist/$(APP)/bootloader.bin
	@cp .pio/build/$(APP)/partitions.bin dist/$(APP)/ptable.bin
	@cp .pio/build/$(APP)/firmware.bin   dist/$(APP)/firmware.bin
	@echo "$(APP) built from $$(git rev-parse --short HEAD)$$(git diff --quiet || echo -dirty)" | tee dist/$(APP)/PROVENANCE
	@ls -l dist/$(APP)

# Re-render every board variant header from its descriptor (needs the SDK: make sdk)
board-headers:
	for d in boards/*.yaml; do .venv/bin/python tools/gen-board-header.py "$$d" \
	  "lib/atech_board/variants/$$(basename $$d .yaml).h"; done

clean:
	rm -rf .pio dist

sdk:
	uv venv .venv -q && uv pip install -q --python .venv/bin/python atech

sync-sdk:
	tools/sync-sdk-modules.sh

check:
	$(ATECH) check --port $(PORT)

# usage: make send KEY=grid_hold VALUE=4
send:
	$(ATECH) send --port $(PORT) $(KEY) '$(VALUE)'

# What the distance sensor sees, from the frames the sid-theremin build prints: ARGS="--max 1600"
grid-view:
	.venv/bin/python tools/grid-view.py --port $(PORT) $(ARGS)

# The MacBook's keyboard as a piano for the sid-theremin build, over Bluetooth MIDI: ARGS='--play "60 64 67"'
keys:
	uv run --script tools/midi-keys.py $(ARGS)

# The minimal ESP-IDF sample: a different framework, its own project
idf-minimal:
	. $(HOME)/esp/esp-idf-v5.5.4/export.sh >/dev/null && cd idf-minimal && idf.py build
