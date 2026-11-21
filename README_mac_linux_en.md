# Rogue 5.2 JEC 004 — macOS / Linux (English)

This document explains how to build, run, and install Rogue 5.2 JEC on **macOS, Linux, and Raspberry Pi OS**, including save and score file locations. For an introduction and links to platform-specific documentation, see the [repository README](README.md). For controls, items, and game options, see [A Guide to the Dungeons of Doom](rogue.md).

日本語版：[README_mac_linux_ja.md](README_mac_linux_ja.md)

## 1. Requirements

Extract the source and open a terminal in its directory.

**macOS:** install Apple's command-line tools:

```sh
xcode-select --install
```

**Ubuntu / Debian / Raspberry Pi OS:**

```sh
sudo apt update
sudo apt install build-essential libncurses-dev
```

**Fedora:** `sudo dnf install gcc make ncurses-devel`  
**Arch Linux:** `sudo pacman -S --needed base-devel ncurses`  

On Linux, running `./configure` before building is recommended:

```sh
./configure
```

On macOS, `make` usually works without configuration. The `configure` script selects a C compiler and tries `ncursesw`, `ncurses`, and `curses`, in that order. Use `--curses-lib=-lncursesw` or another library flag if needed.

## 2. Build

For the standard build:

```sh
make clean
make
```

This produces **`rogue`**. Regular `make` uses convenient defaults; `make original` selects original-style defaults and English startup language. Both builds can switch languages during play.

| Setting | Regular `make` | `make original` |
|---|---|---|
| Startup language | OS language | English |
| `color` (color display) | on | off |
| `passgo` (automatically turn corridor bends) | on | off |
| `askme` (prompt to name unidentified items) | on | off |
| `cursor` (cursor visibility) | off | on |

`on` means enabled and `off` means disabled. These options can also be changed using `ROGUEOPTS` or the in-game `o` command; see the [game guide](rogue.md).

| Purpose | Command |
|---|---|
| Regular build | `make` |
| Original-style defaults, English | `make original` |
| Include wizard mode | `make wizard` |
| Japanese startup language | `make japanese` |
| English startup language | `make english` |
| Name the executable `rogue52` | `./configure --program=rogue52` → `make clean` → `make` |

The `configure` script writes settings to `config.mk`. Additional options include `--default-language=auto|ja|en`, `--enable-wizardmode`, `--curses-lib=LIBS`, and `--prefix=DIR`. Without `configure`, use `make PROGRAM=rogue52` (or `make PROG=rogue52`) to rename the executable; `PROGRAM` takes precedence. If `config.mk` already exists, use `./configure --program=NAME` to update the configured file names.

Clean and rebuild when changing build variants or settings. `make distclean` also removes generated configuration files. **Neither command deletes save or score files.**

## 3. Launch

```sh
./rogue
```

A normal launch starts a new game. Use a terminal at least **80 columns × 24 rows**; the display is fixed at 80×24.

| Command | Purpose |
|---|---|
| `./rogue -r` | Resume the default saved game |
| `./rogue -s` | Display high scores |

You can select a startup language using `ROGUEOPTS`:

```sh
ROGUEOPTS='language=ja' ./rogue
ROGUEOPTS='language=en,askme,passgo' ./rogue
```

`language=auto` follows the OS/terminal locale. UTF-8 strings, including Japanese text, are supported for player names, fruit names, and save-file names, subject to terminal input/display support. See the [game guide](rogue.md) for option details.

## 4. Installation (optional)

You can run Rogue directly from the build directory. To install it using the default locations:

```sh
sudo make install
```

Use `./configure --prefix=DIR` to change the installation prefix and `--program=NAME` to change the executable name. `make install` installs the executable, man page, READMEs, guides, and license. `make uninstall` removes them. Use `make DESTDIR=DIR install` when creating a system package.

## 5. Save and score file locations

By default, the save file **`rogue.save`** and score file **`rogue.scr`** are stored in the process working directory. Changing the executable name normally also changes these names. Score updates lock the score file itself; no separate `.lck` file is created.

To keep both files in `~/Documents`:

```sh
./configure --program=rogue52 --file-dir='~/Documents'
make clean
make
```

The resulting files are `rogue52.save` and `rogue52.scr` under Documents. A leading `~` expands to the home directory. The game user must have write access. `make install` can create a missing `--file-dir` directory, but take care with ownership if running it using `sudo`.

Individual paths can be selected with `--savefile=PATH` and `--scorefile=PATH`; `--savedir=DIR` changes only the save directory. Direct make builds also accept `SAVEDIR=DIR`, `SAVEFILE=PATH`, and `SCOREFILE=PATH`. Rebuild after changing these settings. The old `--lockfile` option is accepted for compatibility but is ignored.

## 6. Files and distribution

| File | Purpose |
|---|---|
| `rogue` | Regular built executable |
| `rogue.save` / `rogue.scr` | Saved game / high scores |
| `configure` / `Makefile` | Build and installation |
| `config.mk` / `config.log` | Generated configuration and log |
| `rogue.6` | man-page source |
| `rogue.md` / `rogue_jp.md` | English and Japanese Markdown guides |
| `rogue_en.txt` / `rogue_ja.txt` | English and Japanese fixed-width TXT guides |
| `*.c` / `*.h` | Source code |

For other platform documentation, see the [repository README](README.md). Run `make dist.src` to create the **`rogue52_004-src.tar.gz`** source distribution; local saves, scores, and `config.mk` are not included.

## 7. Copyright and license

See [LICENSE.TXT](LICENSE.TXT) for copyright and license terms of the original Rogue and existing code, and for the JEC modifications and acknowledgments.
