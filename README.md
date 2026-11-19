# Rogue ADV (M5Stack Cardputer ADV + M5Launcher)

A color CUI-style **Rogue** port for **M5Stack Cardputer ADV** (240×135 ST7789V2 TFT + TCA8418 I2C keyboard) and **M5Launcher**.  
**M5Stack Cardputer ADV + M5Launcher** 環境向けに移植した、カラー表示対応の CUI 風 **Rogue** です。

---

## Features / 主な機能

- **Classic Rogue Gameplay / クラシックな Rogue ゲームプレイ**
  - Procedural multi-room dungeons (`B1F`–`B16F`), Fog of War with explored memory, 16 monster types (`B` Bat to `D` Dragon & `J` Jabberwock), hunger system, traps, secret doors, cursed/enchanted gear, and unidentified potions/scrolls/rings.
  - 毎ラン自動生成される多部屋ダンジョン（`B1F`〜`B16F`）、視界と探索済みマップの霧（Fog of War）、16種類のモンスター、空腹度システム、罠、隠し扉、呪い/強化装備、未識別アイテムの識別システムを実装しています。

- **240×135 Color Viewport & Minimap / カラー表示とミニマップ**
  - Crisp `40×13` scrolling viewport (`6×8` font) over a `60×21` dungeon map, using double-buffered `M5Canvas` rendering with color-coded terrain, items, monster tiers, HP, and status effects.
  - `60×21` のマップを `40×13` のビューポートでスクロール表示し、`M5Canvas` によるダブルバッファリングで地形・アイテム・モンスター脅威度・HP・状態異常をカラー表示します。
  - Press `m` to open the full-floor color pixel minimap (`3×3` px per tile).
  - `m` キーでフロア全体を俯瞰できるカラーピクセル・ミニマップを表示できます。

- **Persistent Records & Replayability (NVS) / やりこみ・自己記録保存**
  - Saved automatically to ESP32 NVS (`Preferences`):
    - **Personal Bests**: Deepest Floor (`Best Floor`), `Top Score`, `Total Runs`, `Total Wins`
    - **Hall of Fame**: Top 8 runs with score, floor reached, and cause of death / victory
    - **10 Unlockable Badges**: `First Blood`, `Deep Diver`, `Abyssal`, `Hoarder`, `Scholar`, `Dragon Slayer`, `Amulet Bearer`, `True Rogue`, `Veteran`, `Survivor`
    - **Live Record Alerts**: In-game `NEW RECORD!` notification when you surpass your deepest floor.
  - ESP32 の NVS（`Preferences`）に以下の記録を永続保存します：
    - **自己ベスト**: 最高到達階層（`Best Floor`）、ハイスコア（`Top Score`）、総プレイ数、クリア数
    - **Hall of Fame**: スコア上位8件のランキング（到達階層・死因/脱出記録付き）
    - **10種類の実績バッジ**: `First Blood`、`Deep Diver`、`Abyssal`、`Hoarder`、`Scholar`、`Dragon Slayer`、`Amulet Bearer`、`True Rogue`、`Veteran`、`Survivor`
    - **リアルタイム新記録通知**: 最高到達階層を更新するとゲーム中に `NEW RECORD!` を通知します。

---

## Controls / 操作方法

| Action / 操作 | Key / キー |
|---|---|
| Move (4-way) / 上下左右移動 | `h` `j` `k` `l` or `Fn` + Arrow (`;` `.` `,` `/`) |
| Move (Diagonal) / 斜め移動 | `y` `u` `b` `n` |
| Rest (1 turn) / 1ターン待機 | `.` |
| Search (traps, secret doors) / 探索 | `s` |
| Stairs Down / Up / 階段の上り下り | `>` / `<` or `Enter` on stairs |
| Inventory / 持ち物一覧 | `i` |
| Quick Inventory / クイック選択 | `e` (Eat) / `q` (Quaff) / `r` (Read) / `w` (Wield) |
| Minimap / ミニマップ切替 | `m` |
| Help / ヘルプ表示 | `?` |
| Pause / Menu / ポーズ・メニュー | `` ` `` (top-left `ESC` key / 左上 `ESC` キー) |

---

## Build & Install for M5Launcher / ビルドと M5Launcher への導入

### Prerequisites / 必要な環境
- [PlatformIO Core](https://platformio.org/) (`pio`)

### Build / ビルド
```bash
pio run
```
The application binary is generated at:  
ビルド後のアプリバイナリは以下のパスに出力されます：
```text
.pio/build/cardputer-adv/firmware.bin
```

### Install to M5Launcher / M5Launcher への導入手順
1. Copy `.pio/build/cardputer-adv/firmware.bin` to your microSD card (you can rename it, e.g. `RogueADV.bin`), or upload it via the M5Launcher WebUI.  
   `.pio/build/cardputer-adv/firmware.bin` を任意の名前（例: `RogueADV.bin`）で microSD カードにコピーするか、M5Launcher の WebUI からアップロードします。
2. Boot **RogueADV** from the **M5Launcher** menu on your Cardputer ADV.  
   Cardputer ADV の M5Launcher メニューから起動してください。

---

## Host Unit Tests / ホスト側ユニットテスト

The core game logic (`src/game/`) has no hardware dependencies and can be tested directly on your PC:  
ゲームロジック（`src/game/`）はハードウェア非依存のため、PC 上で単体テストを実行できます：

```bash
g++ -std=c++17 -Wall -Wextra -Werror -O2 tests/test_rogue.cpp src/game/dungeon.cpp src/game/records.cpp src/game/rogue.cpp -o /tmp/test_rogue && /tmp/test_rogue
```
