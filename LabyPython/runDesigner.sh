#!/bin/sh
set -eu

if command -v designer >/dev/null 2>&1; then
	designer MazeCreator.ui
elif command -v designer-qt6 >/dev/null 2>&1; then
	designer-qt6 MazeCreator.ui
elif command -v qtchooser >/dev/null 2>&1; then
	qtchooser -run-tool=designer -qt=6 MazeCreator.ui
else
	echo "Qt Designer was not found. Rebuild the dev container after installing qt6-tools-dev-tools." >&2
	exit 1
fi

pyuic6 MazeCreator.ui >| src/LabyPython/mazeCreator.py
