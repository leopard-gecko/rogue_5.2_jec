# Rogue 5.2 JEC 004 — Windows（日本語）

この文書では、**Windows 10 / 11**向けのビルド、起動、環境変数、セーブ・スコアの保存先について説明します。ゲームの紹介と各OS向け文書の一覧は[リポジトリのREADME](README.md)、操作方法やアイテムの説明は[「【運命の洞窟】への案内書」](rogue_jp.md)をご覧ください。

English version: [README_windows_en.md](README_windows_en.md)

## 1. 開発環境の準備

ソースからWindows版を作るには **MSYS2 UCRT64** を使用します。<https://www.msys2.org/> からMSYS2をインストールし、スタートメニューの「MSYS2 UCRT64」を開いてください。

```sh
pacman -Syu
```

更新のため再起動が必要と表示されたら、UCRT64を開き直して更新を完了してください。続いて開発ツールとPDCursesModをインストールします。

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-binutils mingw-w64-ucrt-x86_64-pdcurses
```

Windowsでは `./configure` は不要です。ソースコードのフォルダに移動して作業してください。

## 2. ビルド

```sh
make clean
make
```

成功すると **`rogue.exe`** が作成されます。

| 目的 | コマンド |
|---|---|
| 通常ビルド | `make` |
| 実行ファイル名を `rogue52.exe` にする | `make PROGRAM=rogue52`（または `make PROG=rogue52`） |
| オリジナル寄りの設定・英語 | `make original` |
| オリジナル設定版を別名で作る | `make PROGRAM=rogue_original original` |
| ウィザード機能付き | `make wizard` |

通常の `make` は遊びやすい初期設定を使用し、`make original` は原典に近い設定と英語表示を既定にします。どちらもゲーム中に表示言語を切り替えられます。

| 設定 | 通常ビルド `make` | `make original` |
|---|---|---|
| 初期表示言語 | Windowsの表示言語から選択 | 英語 |
| `color`（カラー） | on | off |
| `passgo`（通路の角で自動移動） | on | off |
| `askme`（未識別の品物の呼び名を尋ねる） | on | off |
| `cursor`（カーソル表示） | off | on |

`on` は有効、`off` は無効です。これらの項目は `ROGUEOPTS` やゲーム中の `o` コマンドで変更できます。詳しくは[案内書](rogue_jp.md)をご覧ください。ビルド設定を変更するときは `make clean` してから作り直してください。

Windows版は**PDCursesModのVTバックエンドのみ**を使用します。表示色や絵文字の見え方はターミナルとフォントに依存します。

## 3. 起動と終了

**`rogue.exe`をダブルクリック**するとゲームが始まります。既定のセーブファイルがある場合は、再開・新規・別のセーブファイルを選択・終了の起動メニューが表示されます。セーブファイルを実行ファイルへドラッグ＆ドロップして、そのファイルから再開することもできます。日本語や空白を含むファイルパスにも対応しています。

既存のPowerShellやコマンドプロンプトから起動することもできます。PowerShellでは次のように指定します。

```powershell
.\rogue.exe
```

既存のシェルから引数なしで起動すると、新規ゲームが始まります。ターミナルは**80列×24行以上**必要です。ダブルクリック起動時にはWindows TerminalまたはConsole Hostを80×24に調整するよう試みますが、端末によっては調整できない場合があります。既存のシェルから起動した場合は、サイズを変更しません。

| コマンド | 内容 |
|---|---|
| `.\rogue.exe -r` | 既定のセーブファイルから再開する |
| `.\rogue.exe -s` | スコア一覧を表示する |
| `.\rogue.exe "保存ファイル名"` | 指定したセーブファイルから再開する |

ゲーム中の `!` コマンドは一時的にWindowsのコマンドシェルを開きます。`exit` で終了するとゲームに戻ります。通常のゲーム内容やオプションの説明は[案内書](rogue_jp.md)をご覧ください。

## 4. Windowsでの環境変数の設定

ゲーム起動時の設定には **`ROGUEOPTS`** を使用できます。設定できる項目の意味は[案内書の「オプション」](rogue_jp.md)をご覧ください。

**PowerShellで一時的に指定：**

```powershell
$env:ROGUEOPTS = 'language=ja,askme,passgo'
.\rogue.exe
```

**コマンドプロンプト（cmd.exe）で指定：**

```cmd
set "ROGUEOPTS=language=ja,askme,passgo"
rogue.exe
```

**MSYS2 UCRT64で指定：**

```sh
export ROGUEOPTS='language=ja,askme,passgo'
./rogue.exe
```

これらの指定は、その端末とそこから起動したプログラムに有効です。エクスプローラーからのダブルクリックには引き継がれません。

常に使用したい場合は、`Win + R` → `sysdm.cpl` →「詳細設定」→「環境変数」→「ユーザー環境変数」で、変数名 `ROGUEOPTS` と設定文字列を登録します。設定後は新しい端末を開き、必要ならWindowsに再サインインしてください。使わなくなった場合は登録を削除します。

## 5. セーブ・スコアなどの保存先

通常の `make` で作った **`rogue.exe`** は、既定で作業ディレクトリの **`rogue.save`** と **`rogue.scr`** を使います。スコアファイルをOSの機能でロックするため、専用の `.lck` ファイルは作成しません。`SAVEFILE=PATH`、`SCOREFILE=PATH`、`SAVEDIR=DIR` をビルド時に指定できます。相対パスは起動時の作業ディレクトリからの相対指定です。

**Windows配布用バイナリ**は、通常版の `rogue52.exe` とオリジナル設定版の `rogue52_original.exe` が、共通で `rogue52.save` と `rogue52.scr` を使用します。同じセーブデータを複数プロセスで同時に使用しないでください。スコア更新時は排他制御が働きます。

個人用ビルドで別の保存先を使う場合は、例えば次のように指定できます（保存先ディレクトリは事前に作成してください）。

```sh
make clean
make PROGRAM=rogue52 SAVEDIR='C:/Games/Rogue52' SCOREFILE='C:/Games/Rogue52/rogue52.scr'
```

他のPCへ配布する場合は、個人の環境に依存する絶対パスを埋め込まないでください。配布用ZIPは作業ディレクトリを使用する設定で生成されます。

## 6. 配布ファイルと説明書

Windows用のバイナリ配布ZIPを生成するには、MSYS2 UCRT64で次を実行します。

```sh
pacman -S --needed zip
make dist.win
```

既定では **`rogue52_004-windows.zip`** を生成します。ZIP内には通常版 `rogue52.exe`、オリジナル設定版 `rogue52_original.exe`、日英の利用者用README、日英のTXT案内書（`rogue_ja.txt` / `rogue_en.txt`）、`LICENSE.TXT` が収録されます。2つの実行ファイルはそれぞれ異なる初期設定で再コンパイルされます。セーブ・スコア名は共通です。両方のEXEには同じアイコンが埋め込まれます。

```sh
make dist.win PROGRAM=rogue52
```

`PROGRAM=NAME`（または `PROG=NAME`）で実行ファイル名を変更すると、通常版は `NAME.exe`、オリジナル設定版は `NAME_original.exe` となり、共通のデータ名は `NAME.save` と `NAME.scr` に変わります。配布READMEと案内書の起動例も自動変更されます。元の文書は変更されません。

ソース配布用アーカイブは `make dist.src` で生成します（**`rogue52_004-src.tar.gz`**）。

| ファイル | 用途 |
|---|---|
| `rogue.exe` | 通常のソースビルドで生成される実行ファイル |
| `rogue52.exe` / `rogue52_original.exe` | バイナリ配布用の2種類の実行ファイル |
| `rogue.save` / `rogue.scr` | 通常ビルドのセーブ／スコア |
| `Makefile` / `Makefile.win` | ソースからのビルド |
| `windows/rogue.ico` / `windows/rogue-icon.rc` | Windows実行ファイルへ埋め込むアイコンとリソース定義 |
| `rogue_ja.txt` / `rogue_en.txt` | バイナリ配布に収録する日英TXT案内書 |
| `windows/README_binary_ja.txt` / `windows/README_binary_en.txt` | バイナリ配布用READMEの原稿 |

案内書とほかのOS向けREADMEは[リポジトリのREADME](README.md)をご覧ください。

## 7. 著作権・ライセンス

オリジナルRogueと既存コードの著作権・ライセンス条件、JEC版での日本語化・機能追加については、[LICENSE.TXT](LICENSE.TXT)をご覧ください。
