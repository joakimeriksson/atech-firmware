#!/bin/sh
# Copy the real Atech driver sources from the installed `atech` Python SDK into
# lib/atech_<module>/ — the same layout `atech build` generates.
# Re-run after `uv pip install -U atech` to pick up driver updates.
set -e
cd "$(dirname "$0")/.."
PY=.venv/bin/python
PKG=$($PY -c "import atech,os; print(os.path.dirname(atech.__file__))")
VER=$($PY -c "import importlib.metadata as m; print(m.version('atech'))")
for m in speaker st7735_tft rotary_encoder button neopixel icm40608 distance_sensor; do
  rm -rf lib/atech_$m; mkdir -p lib/atech_$m
  cp "$PKG/catalog/data/modules/$m/"*.h "$PKG/catalog/data/modules/$m/"*.cpp lib/atech_$m/
  echo "synced $m"
done
echo "atech==$VER" > lib/SDK_VERSION
echo "drivers from atech SDK $VER"
