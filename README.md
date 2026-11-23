# Rogue 5.2 JEC

**Rogue 5.2 JEC 005**

Rogueは、1980年頃に誕生した、文字や記号で描かれた迷宮を探索するコンピューターRPGです。怪物と戦い、道具を使いながら、迷宮の奥に隠された「イェンダーの魔除け」を探します。Rogue 5.2は初期のRogueを伝えるバージョンの一つです。

**Rogue 5.2 JEC** は、Rogue 5.2のゲーム内容をできるだけ維持しながら、日本語・英語の切り替え、カラー表示、現代のOSへの対応などを加えた修正版です。

*Rogue 5.2 JEC is a modified edition of Rogue 5.2. It aims to preserve the original gameplay while adding Japanese/English language switching, color display, and support for modern operating systems.*

## 対応環境とREADME / Platforms and documentation

macOS、Linux（Raspberry Pi OSを含む）、Windows 10 / 11に対応しています。ビルド方法、起動方法、初期設定、セーブ・スコアの保存先は、OS・言語別のREADMEをご覧ください。

*For build instructions, launching, default options, and save/score file locations, select the README for your operating system and language.*

| 対象OS / Platform | 日本語 | English |
| --- | --- | --- |
| macOS / Linux / Raspberry Pi OS | [README（日本語）](README_mac_linux_ja.md) | [README (English)](README_mac_linux_en.md) |
| Windows 10 / 11 | [README（日本語）](README_windows_ja.md) | [README (English)](README_windows_en.md) |

macOS / Linuxでは通常 `make`（必要に応じて `./configure`）、WindowsではMSYS2 UCRT64からビルドできます。詳しい準備は各READMEを参照してください。

*On macOS/Linux, use `make` (and `./configure` when needed). Windows builds use MSYS2 UCRT64; see the platform-specific README for details.*

## 【運命の洞窟】への案内書 / A Guide to the Dungeons of Doom

ゲームの操作、画面表示、アイテム、オプションなどは、日英の案内書にまとめています。GitHubで閲覧しやすいMarkdown版と、5.4と同様の固定幅TXT版を同梱しています。

*The Japanese and English dungeon guides cover commands, items, display symbols, and options. Both Markdown and fixed-width plain-text editions are included.*

| 言語 / Language | Markdown | TXT |
| --- | --- | --- |
| 日本語 | [rogue_jp.md](rogue_jp.md) | [rogue_ja.txt](rogue_ja.txt) |
| English | [rogue.md](rogue.md) | [rogue_en.txt](rogue_en.txt) |

ブラウザでは[公開サイトの案内書（5.2）](https://leopard-gecko.github.io/jrogue/guide.html?version=52)も利用できます。

*An [online guide for Rogue 5.2](https://leopard-gecko.github.io/jrogue/guide.html?version=52) is also available.*

## 著作権・ライセンス / Copyright and license

オリジナルRogueと既存のコードに関する著作権・ライセンス、JEC版での変更と謝辞については、[LICENSE.TXT](LICENSE.TXT)をご覧ください。

*See [LICENSE.TXT](LICENSE.TXT) for copyright notices, license terms, modifications, and acknowledgments.*
