# Rogue 5.2 JEC 004 — macOS / Linux（日本語）

この文書では、**macOS、Linux、Raspberry Pi OS**でのビルド、起動、インストール、セーブ・スコアの保存先について説明します。ゲームの紹介とOS別の文書一覧は[リポジトリのREADME](README.md)、操作方法やアイテムの説明は[「【運命の洞窟】への案内書」](rogue_jp.md)をご覧ください。

English version: [README_mac_linux_en.md](README_mac_linux_en.md)

## 1. 開発環境の準備

ソースコードを展開し、ターミナルでそのディレクトリを開きます。

**macOS：** Appleのコマンドラインツールをインストールします。

```sh
xcode-select --install
```

**Ubuntu / Debian / Raspberry Pi OS：**

```sh
sudo apt update
sudo apt install build-essential libncurses-dev
```

**Fedora：** `sudo dnf install gcc make ncurses-devel`  
**Arch Linux：** `sudo pacman -S --needed base-devel ncurses`  

Linuxでは、ビルド前に `./configure` を実行してcursesライブラリを検出することを推奨します。

```sh
./configure
```

macOSでは一般に `make` だけでもビルドできます。`./configure` は使用するコンパイラと、`ncursesw`、`ncurses`、`curses` の順で利用可能なライブラリを検出します。必要に応じて `--curses-lib=-lncursesw` などを指定できます。

## 2. ビルド

通常のビルドは次のとおりです。

```sh
make clean
make
```

成功すると **`rogue`** が作成されます。通常の `make` は遊びやすい初期設定を使用し、`make original` は原典に近い設定と英語表示を既定にします。どちらもゲーム中に言語を切り替えられます。

| 設定 | 通常ビルド `make` | `make original` |
|---|---|---|
| 初期表示言語 | OSの設定から選択 | 英語 |
| `color`（カラー） | on | off |
| `passgo`（通路の角で自動移動） | on | off |
| `askme`（未識別の品物の呼び名を尋ねる） | on | off |
| `cursor`（カーソル表示） | off | on |

`on` は有効、`off` は無効です。これらの項目は `ROGUEOPTS` やゲーム中の `o` コマンドで変更できます。詳しくは[案内書](rogue_jp.md)をご覧ください。

| 目的 | コマンド |
|---|---|
| 通常ビルド | `make` |
| オリジナル寄りの設定・英語 | `make original` |
| ウィザード機能付き | `make wizard` |
| 日本語を初期言語にする | `make japanese` |
| 英語を初期言語にする | `make english` |
| 実行ファイル名を `rogue52` にする | `./configure --program=rogue52` → `make clean` → `make` |

`./configure` による設定は `config.mk` に保存されます。`--default-language=auto|ja|en`、`--enable-wizardmode`、`--curses-lib=LIBS`、`--prefix=DIR` なども利用できます。`configure` を使用しない場合は `make PROGRAM=rogue52`（または `make PROG=rogue52`）でも名前を変更できます。両方指定した場合は `PROGRAM` が優先されます。`config.mk` がある場合、名前の変更は `./configure --program=NAME` で行ってください。

ビルド方式や設定を変更する場合は、必要に応じて `make clean` から作り直してください。`make distclean` は `config.mk` などの設定ファイルも削除します。**セーブ・スコアファイルは削除しません。**

## 3. 起動

```sh
./rogue
```

通常起動では新しいゲームが始まります。ターミナルは**80列×24行以上**を用意してください。表示範囲は80×24に固定されています。

| コマンド | 内容 |
|---|---|
| `./rogue -r` | 既定のセーブファイルから再開する |
| `./rogue -s` | スコア一覧を表示する |

表示言語は起動時に `ROGUEOPTS` で指定できます。

```sh
ROGUEOPTS='language=ja' ./rogue
ROGUEOPTS='language=en,askme,passgo' ./rogue
```

`language=auto` ならOS・端末の言語設定から選択します。日本語の名前や果物名、セーブファイル名などのUTF-8文字列にも対応します（表示・入力は使用する端末によります）。オプション名の詳細は[案内書](rogue_jp.md)をご覧ください。

## 4. インストール（任意）

ソースディレクトリから直接実行する場合、インストールは不要です。通常は次のようにインストールできます。

```sh
sudo make install
```

インストール先は `./configure --prefix=DIR`、実行ファイル名は `--program=NAME` で変更できます。`make install` は実行ファイル、manページ、README、案内書、ライセンスをインストールします。`make uninstall` で削除できます。パッケージを作る場合は `make DESTDIR=DIR install` に対応しています。

## 5. セーブ・スコアなどの保存先

既定では、セーブファイル **`rogue.save`** とスコアファイル **`rogue.scr`** を、起動時の作業ディレクトリに作成します。実行ファイル名を変えると既定名も通常は連動します。スコアファイル自体をOSの機能でロックし、専用の `.lck` ファイルは作りません。

両方を `~/Documents` に置く例です。

```sh
./configure --program=rogue52 --file-dir='~/Documents'
make clean
make
```

この例では、`rogue52.save` と `rogue52.scr` をDocumentsに保存します。先頭の `~` はホームディレクトリに展開されます。保存先はゲームを実行するユーザーが書き込める必要があります。`make install` は `--file-dir` で指定したディレクトリを必要に応じて作成しますが、`sudo` で作る際は所有者と書き込み権限に注意してください。

個別のファイル名は `--savefile=PATH` と `--scorefile=PATH`、セーブディレクトリだけなら `--savedir=DIR` でも指定できます。直接 `make` で指定する場合は `SAVEDIR=DIR`、`SAVEFILE=PATH`、`SCOREFILE=PATH` などが利用できます。変更後は再ビルドしてください。古い `--lockfile` オプションは互換性のため受け付けますが使用しません。

## 6. ファイルの説明と配布

| ファイル | 用途 |
|---|---|
| `rogue` | 通常ビルドの実行ファイル |
| `rogue.save` / `rogue.scr` | セーブ／スコア |
| `configure` / `Makefile` | ビルドとインストール |
| `config.mk` / `config.log` | `configure` により生成される設定とログ |
| `rogue.6` | manページの原稿 |
| `rogue_jp.md` / `rogue.md` | 日英案内書（Markdown） |
| `rogue_ja.txt` / `rogue_en.txt` | 日英案内書（固定幅TXT） |
| `*.c` / `*.h` | ソースコード |

ほかのOS向けREADMEは[リポジトリのREADME](README.md)から参照してください。`make dist.src` でソース配布用の **`rogue52_004-src.tar.gz`** を生成できます。ソース配布にはローカルのセーブ・スコアや `config.mk` は入りません。

## 7. 著作権・ライセンス

オリジナルRogueと既存コードの著作権・ライセンス条件、JEC版での日本語化や機能追加については、[LICENSE.TXT](LICENSE.TXT)をご覧ください。
