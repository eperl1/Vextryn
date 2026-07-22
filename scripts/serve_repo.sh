#!/bin/bash
REPO_DIR="/tmp/vxpkg_repo"
PORT=${1:-8080}
mkdir -p "$REPO_DIR"
cat > "$REPO_DIR/index.json" << 'JSON'
{
  "repo": "Vextryn Air Official",
  "version": "1",
  "packages": [
    {"name": "vxterm", "version": "1.1", "desc": "Terminal Emulator", "size": 51200, "url": "vxterm-1.1.vxpkg", "sha256": "abc123"},
    {"name": "vxfiles", "version": "1.1", "desc": "File Manager", "size": 38400, "url": "vxfiles-1.1.vxpkg", "sha256": "def456"},
    {"name": "vxedit", "version": "1.1", "desc": "Text Editor", "size": 32768, "url": "vxedit-1.1.vxpkg", "sha256": "ghi789"},
    {"name": "vxweb", "version": "0.1", "desc": "Web Browser", "size": 65536, "url": "vxweb-0.1.vxpkg", "sha256": "jkl012"},
    {"name": "vxcalc", "version": "1.0", "desc": "Calculator", "size": 24576, "url": "vxcalc-1.0.vxpkg", "sha256": "mno345"}
  ]
}
JSON
echo "Package repo index created"
echo "Starting HTTP server on port $PORT..."
cd "$REPO_DIR"
python3 -m http.server "$PORT"
