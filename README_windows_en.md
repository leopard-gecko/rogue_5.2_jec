# Rogue 5.2 JEC 004 — Windows (English)

This document explains how to build and run Rogue 5.2 JEC on **Windows 10 / 11**, including environment variables and save and score files. For an introduction and the list of platform documents, see the [repository README](README.md). For controls, commands, and items, see [A Guide to the Dungeons of Doom](rogue.md).

日本語版：[README_windows_ja.md](README_windows_ja.md)

## 1. Requirements

To build native Windows binaries, use **MSYS2 UCRT64**. Install MSYS2 from <https://www.msys2.org/> and launch “MSYS2 UCRT64” from the Start menu.

```sh
pacman -Syu
```

If prompted, reopen UCRT64 and complete the update. Then install the compiler, tools, and PDCursesMod:

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-binutils mingw-w64-ucrt-x86_64-pdcurses
```

Change to the source directory. Windows builds do not use `./configure`.

## 2. Build

```sh
make clean
make
```

This produces **`rogue.exe`**.

| Purpose | Command |
|---|---|
| Regular build | `make` |
| Build as `rogue52.exe` | `make PROGRAM=rogue52` (or `make PROG=rogue52`) |
| Original-style defaults, English | `make original` |
| Build a separately named original-style EXE | `make PROGRAM=rogue_original original` |
| Include wizard mode | `make wizard` |

Regular `make` uses convenient defaults, while `make original` selects original-style defaults and English on startup. Both builds can switch languages during play.

| Setting | Regular `make` | `make original` |
|---|---|---|
| Startup language | Windows UI language | English |
| `color` (color display) | on | off |
| `passgo` (turn automatically at corridor bends) | on | off |
| `askme` (prompt to name unidentified items) | on | off |
| `cursor` (cursor visibility) | off | on |

`on` means enabled; `off` means disabled. You can also change these options using `ROGUEOPTS` or the in-game `o` command; see the [game guide](rogue.md). Clean and rebuild when changing build settings.

Windows builds use the **PDCursesMod VT backend** exclusively. Colors and emoji rendering depend on your terminal and font.

## 3. Launch and exit

**Double-click `rogue.exe`** in File Explorer to start. If a default saved game exists, a launch menu offers resume, new game, choose another save file, or quit. You can also drag and drop a saved-game file onto the EXE to restore it directly. File paths containing Japanese characters and spaces are supported.

You may also launch from PowerShell or Command Prompt. In PowerShell:

```powershell
.\rogue.exe
```

Launching without arguments from an existing shell starts a new game. The terminal must support at least **80 columns × 24 rows**. Direct double-click launches attempt to resize Windows Terminal or Console Host to 80×24. Launching from an existing shell does not resize its window. Some terminal hosts may ignore the resize request.

| Command | Purpose |
|---|---|
| `.\rogue.exe -r` | Resume the default saved game |
| `.\rogue.exe -s` | Display high scores |
| `.\rogue.exe "saved_filename"` | Resume a specified saved game |

Press `!` during play to temporarily open a Windows command shell. Type `exit` to return to the game. For gameplay options, consult the [game guide](rogue.md).

## 4. Setting environment variables on Windows

Rogue reads startup settings from **`ROGUEOPTS`**. The available game options are described in the [guide's Options section](rogue.md).

**Temporary PowerShell setting:**

```powershell
$env:ROGUEOPTS = 'language=ja,askme,passgo'
.\rogue.exe
```

**Command Prompt (`cmd.exe`):**

```cmd
set "ROGUEOPTS=language=ja,askme,passgo"
rogue.exe
```

**MSYS2 UCRT64:**

```sh
export ROGUEOPTS='language=ja,askme,passgo'
./rogue.exe
```

These settings affect the current shell and programs launched from it, not an unrelated shell or Explorer double-click launch.

To make the setting persistent, press `Win + R`, run `sysdm.cpl`, and open **Advanced → Environment Variables → User variables**. Add a variable named `ROGUEOPTS` with your chosen value. Open a fresh terminal afterward; sign out and sign back in if needed. Remove the variable when no longer required.

## 5. Save and score file locations

A normal **`rogue.exe`** built with `make` uses **`rogue.save`** and **`rogue.scr`** in the process working directory. Score updates lock the score file itself; no extra `.lck` file is created. Use build variables such as `SAVEFILE=PATH`, `SCOREFILE=PATH`, or `SAVEDIR=DIR` to change these paths. Relative paths refer to the process working directory.

The **Windows binary distribution** includes `rogue52.exe` and `rogue52_original.exe`, both using **`rogue52.save`** and **`rogue52.scr`**. Do not operate on the same saved game concurrently. Score updates are protected by OS file locking.

For a personal build, use a separate data directory (create it before building):

```sh
make clean
make PROGRAM=rogue52 SAVEDIR='C:/Games/Rogue52' SCOREFILE='C:/Games/Rogue52/rogue52.scr'
```

Do not embed personal absolute directories in executables intended for distribution. The standard distribution uses working-directory-relative paths.

## 6. Distributed files and guides

Create the Windows binary ZIP in MSYS2 UCRT64:

```sh
pacman -S --needed zip
make dist.win
```

This produces **`rogue52_004-windows.zip`**, containing the standard executable `rogue52.exe`, original-settings `rogue52_original.exe`, Japanese and English end-user READMEs, the Japanese and English plain-text dungeon guides (`rogue_ja.txt` / `rogue_en.txt`), and `LICENSE.TXT`. The two EXEs are built separately with different option defaults, but share their save/score filenames. Both executables embed the same Windows icon.

```sh
make dist.win PROGRAM=rogue52
```

To change the EXE names, use `PROGRAM=NAME` (or `PROG=NAME`). The distribution then contains `NAME.exe` and `NAME_original.exe`, both using `NAME.save` and `NAME.scr`. Command examples in the packaged READMEs and guides are updated automatically without modifying the source documents.

To package the source, run `make dist.src` (creates **`rogue52_004-src.tar.gz`**).

| File | Purpose |
|---|---|
| `rogue.exe` | Regular executable built from source |
| `rogue52.exe` / `rogue52_original.exe` | Standard and original-settings distribution executables |
| `rogue.save` / `rogue.scr` | Default save and score files for regular builds |
| `Makefile` / `Makefile.win` | Source build rules |
| `windows/rogue.ico` / `windows/rogue-icon.rc` | Windows icon and resource definition embedded in both executables |
| `rogue_ja.txt` / `rogue_en.txt` | Plain-text guides included in the binary ZIP |
| `windows/README_binary_ja.txt` / `windows/README_binary_en.txt` | End-user README templates for binary distribution |

For guides and other platform READMEs, see the [repository README](README.md).

## 7. Copyright and license

See [LICENSE.TXT](LICENSE.TXT) for copyright and license terms of original Rogue and pre-existing code, and for JEC modifications and acknowledgments.
