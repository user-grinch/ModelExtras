#!/bin/bash

# Define paths
PROJECT_ROOT=$(pwd)
BUILD_DIR="$PROJECT_ROOT/build/mingw/x86"
RES_DIR="$PROJECT_ROOT/resource/dist"
STAGING_DIR="$PROJECT_ROOT/archive_staging"
OUTPUT_NAME="archive/ModelExtras_$(date +%d-%m-%y)"

# 1. Clean up old staging/archives
rm -rf "$STAGING_DIR"
rm -f "${OUTPUT_NAME}.7z."*

# 2. Create directory structure in staging
mkdir -p "$STAGING_DIR/debug"
mkdir -p "$STAGING_DIR/release"
mkdir -p "$STAGING_DIR/resources"

echo ">> Gathering files..."

# 3. Copy specific items
# Debug files
cp "$BUILD_DIR/debug/ModelExtras.asi" "$STAGING_DIR/debug/"
cp "$BUILD_DIR/debug/ModelExtras.dll.a" "$STAGING_DIR/debug/"

# Release files
cp "$BUILD_DIR/release/ModelExtras.asi" "$STAGING_DIR/"
cp "$BUILD_DIR/release/ModelExtras.dll.a" "$STAGING_DIR/"

# Resources
if [ -d "$RES_DIR" ]; then
    cp -r "$RES_DIR/." "$STAGING_DIR/"
else
    echo "Warning: $RES_DIR not found!"
fi

echo ">> Creating 8MB split archive..."

# 4. Create split archive
# -v8m: Split into 8 Megabyte chunks
# -mx9: Ultra compression
# -t7z: 7z format
7z a -v8m -mx9 "${OUTPUT_NAME}.7z" "$STAGING_DIR/*"

# 5. Cleanup staging
rm -rf "$STAGING_DIR"

echo ">> Done! Created volumes:"
ls -lh ${OUTPUT_NAME}.7z.*