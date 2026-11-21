Rogue 5.2 JEC (@DISTNAME@)  Windows Edition
=============================================

Rogue is a computer role-playing game that originated around 1980. You
explore a dungeon drawn with characters and symbols, fight monsters,
and use various items while searching for the Amulet of Yendor.

Rogue is also known as the game that gave rise to the genre now called
"roguelikes." Its dungeons change with every game, you discover the
properties of unfamiliar items, and every decision can affect the
outcome of your adventure.

This package contains Rogue 5.2 JEC (@DISTNAME@) for Windows 10 / 11
(64-bit). It preserves the basic gameplay of Rogue 5.2 while adding
Japanese/English language switching, Japanese and emoji input, color
display, and support for modern terminals and operating systems.

[Included Files]

  @PROGRAM@.exe                 Standard edition
  @ORIGINAL_PROGRAM@.exe        Original-settings edition
  rogue_ja.txt                Guide to the Dungeons of Doom (Japanese)
  rogue_en.txt                A Guide to the Dungeons of Doom (English)
  LICENSE.TXT                 Copyright and license information
  README_ja.txt               Japanese README
  README_en.txt               This file (English README)

The standard and original-settings editions use the same gameplay but
have different startup defaults for language, color, passage-following,
asking to name unidentified items, and cursor display.
Both editions allow you to switch languages during a game.

  Option                      Standard             Original settings
  -------------------------------------------------------------------
  Startup language            System locale        English
  Color                       On                   Off
  Follow corridor bends       On                   Off
  Ask to name unknown items   On                   Off
  Cursor                      Off                  On

Language, color, corridor-following, item naming prompts, cursor, and other options can
also be changed in the game. Both executables share the same save file and high scores.

[Starting the Game]

1. Extract the ZIP archive to a folder of your choice. Use a folder
   where you have write access so the game can create save files.
   (Example: C:\Games\Rogue)

2. In File Explorer, double-click @PROGRAM@.exe for the standard
   edition or @ORIGINAL_PROGRAM@.exe for the original-settings edition.

   If the default save file exists, a launch menu appears:

     [R/Enter] Resume saved adventure
     [N]       Start a new adventure
     [F]       Choose another save file
     [Q]       Quit

   If there is no default save, a new game starts without the menu.
   Pressing F opens the Windows file picker; cancelling it returns to
   the menu. Starting a new game does not immediately delete an
   existing save file.

3. You can drag and drop a saved-game file onto @PROGRAM@.exe or
   @ORIGINAL_PROGRAM@.exe to resume that file directly.

4. To launch from a terminal, open the extracted folder. On Windows 11,
   right-click and select "Open in Terminal". If that is unavailable,
   enter powershell in the folder's address bar and press Enter. Run:

   Standard edition:
       .\@PROGRAM@.exe

   Original-settings edition:
       .\@ORIGINAL_PROGRAM@.exe

   You can also use Command Prompt (cmd.exe). Launching without an
   argument from an existing terminal starts a new game.

Use a terminal at least 80 columns wide and 24 rows high. The dungeon
display is fixed at 80x24. On a direct double-click launch, the program
attempts to resize Windows Terminal or Console Host to 80x24. Some
terminals may not honor that request. It does not resize a terminal
from which you launch the game manually.

During a game, press ! to open a shell. Type exit to return to Rogue.

[Save Files and Scores]

Press S during a game to save your adventure. To resume the default
saved game from a terminal, run:

       .\@PROGRAM@.exe -r

To resume a save file with another name or location, specify its path:

       .\@PROGRAM@.exe "save_filename"

To display the high-score list only:

       .\@PROGRAM@.exe -s

For the original-settings edition, substitute @ORIGINAL_PROGRAM@.exe
for @PROGRAM@.exe in the commands above.
After a successful restore, the original save file is deleted by
design. Copy any saved game you want to keep before resuming it.

Both editions share these default file names:

  Save file    @PROGRAM@.save
  Score file   @PROGRAM@.scr

These files are resolved relative to the working directory at launch.
For direct File Explorer launches, that is normally the executable's
folder. Both executables can resume the same saved game and share high
scores. Avoid using the same saved game in multiple running instances.

After a direct double-click launch, the final score list stays visible
until you press Enter. This extra pause is not added for launches from
an existing terminal.

You can change the default save filename with file= in ROGUEOPTS.

[Setting Environment Variables in Windows]

The ROGUEOPTS environment variable can configure startup options. For
available settings and their meanings, see the "Options" section of
the included game guide. This section explains only how to set the
variable in Windows.

Example of a temporary setting in PowerShell:

       $env:ROGUEOPTS = "name=Alex,language=ja"
       .\@PROGRAM@.exe

Example in Command Prompt (cmd.exe):

       set "ROGUEOPTS=name=Alex,language=ja"
       @PROGRAM@.exe

These settings apply to the current terminal and programs launched
from it. They do not automatically apply to File Explorer double-click
launches.

To use the same settings every time, add ROGUEOPTS to your Windows user
environment variables:

  1. Press Win + R, enter sysdm.cpl, and press Enter.
  2. Open "Advanced" -> "Environment Variables".
  3. Under "User variables", select "New" (or "Edit" if it exists).
  4. Enter ROGUEOPTS as the variable name and your settings as its value.
  5. Save with "OK". If needed, sign out and sign back in to Windows.

Delete the user environment variable when you no longer need it.

[Game Guide and License]

For game controls, screen symbols, commands, items, and options, see
the included "Guide to the Dungeons of Doom" (rogue_en.txt or
rogue_ja.txt). These plain-text guides can be read in Notepad.
A monospaced font is recommended for aligned figures and tables.

See LICENSE.TXT for copyright and license information on the original
Rogue, pre-existing code, and JEC localization and other additions.
