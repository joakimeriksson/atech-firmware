# Firmware for the Atech modular boards. One build = an app from src/ plus a board header.
#
#   make build APP=pocket-synth      build one app          (default APP: pocket-synth)
#   make flash APP=grid-selftest     build and flash it
#   make monitor                     serial console
#   make list                        the builds this repo defines
#   make dist APP=pocket-synth       collect bootloader, partition table and app into dist/
#   make sdk / make sync-sdk         install the open Atech SDK and re-copy the real drivers
#   make check / make send KEY=.. VALUE=..   talk to a connected board through the SDK
#
# Needs: platformio (`pio`), uv. PORT defaults to the first ESP32-S3 CDC device.

APP   ?= pocket-synth
PORT  ?= $(firstword $(wildcard /dev/cu.usbmodem*) /dev/cu.usbmodem101)
ATECH  = .venv/bin/atech

.PHONY: build flash monitor list dist clean sdk sync-sdk check send idf-minimal

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

# The minimal ESP-IDF sample: a different framework, its own project
idf-minimal:
	. $(HOME)/esp/esp-idf-v5.5.4/export.sh >/dev/null && cd idf-minimal && idf.py build
