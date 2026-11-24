# mrubyc-zephyr サンプル（nRF54L15-DK）

[mrubyc-zephyr](https://github.com/OpenBlink/mrubyc-zephyr) を nRF Connect SDK（NCS）v3.4.2 の Zephyr モジュールとして組み込み、mruby/c で 2 つのタスクを動かすサンプルです。対象ボードは nRF54L15-DK です。

mruby/c のスケジューラ上で、LED0 を 150 ms 間隔で点滅するタスクと、UART に `heartbeat N` を 1 秒ごとに出力するタスクが動きます。

## 前提

NCS v3.4.2 と対応するツールチェーンが必要です（`nrfutil sdk-manager` で導入）。ターゲットは `nrf54l15dk/nrf54l15/cpuapp` です。mruby 4.0.0 の `mrbc` は `src/*.rb` を変更して再生成するときだけ使います。

## 取得

このリポジトリは west のマニフェストリポジトリです。次のコマンドで NCS 一式と mrubyc-zephyr を取得します。

```sh
west init -m https://github.com/uist1idrju3i/temporary -mr mrubyc/zephyr-ncs mrubyc-workspace
cd mrubyc-workspace
west update
```

`west.yml` が `sdk-nrf` を v3.4.2、`mrubyc-zephyr` を v4.0.0 に固定し、`import: true` で NCS の依存もあわせて取得します。取得後、このリポジトリはワークスペース内の `mrubyc-ncs-sample/` に置かれます。

## ビルドと書き込み

ワークスペースのトップで実行します。

```sh
west build -b nrf54l15dk/nrf54l15/cpuapp mrubyc-ncs-sample
west flash
```

## 動作

UART（115200 bps）に次のように出力され、LED0 が点滅します。

```text
*** Booting nRF Connect SDK v3.4.2 ***
mruby/c on nRF54L15-DK
heartbeat 1
heartbeat 2
...
```

## コードの構成

| ファイル | 役割 |
|---|---|
| `west.yml` | west マニフェスト（sdk-nrf v3.4.2、mrubyc-zephyr v4.0.0） |
| `prj.conf` | `CONFIG_MRUBYC=y` などの Kconfig |
| `src/main.c` | `mrbc_init` → クラス登録 → `mrbc_create_task` → `mrbc_run` |
| `src/led.c` | `Led` クラスの C バインディング（`on` / `off` / `on?`） |
| `src/blink.rb` | LED 点滅タスク（`Led.new(0)`） |
| `src/heartbeat.rb` | ハートビートタスク |
| `src/*_mrb.h` | `mrbc` が生成したバイトコードヘッダー |
| `boards/` | QEMU 用と native_sim 用の devicetree overlay |

mrubyc-zephyr の使い方の要点は、`src/main.c` の 4 ステップです。

1. `mrbc_init(pool, size)` でヒープを初期化する
2. `register_led_class(NULL)` で C のクラスを登録する
3. `mrbc_create_task(bytecode, NULL)` で mruby/c タスクを作る
4. `mrbc_run()` でスケジューラを回す

## Ruby コードの再生成

`.rb` を変更したら `mrbc` でヘッダーを作り直します。

```sh
mrbc -Bblink_mrb --remove-lv -o src/blink_mrb.h src/blink.rb
mrbc -Bheartbeat_mrb --remove-lv -o src/heartbeat_mrb.h src/heartbeat.rb
```

## エミュレータで試す

実機がなくても QEMU か native_sim で動作を確認できます。

```sh
# native_sim（推奨。タイマーはほぼ実時間どおり）
west build -b native_sim/native mrubyc-ncs-sample -- -DCONFIG_GPIO_EMUL=y
./build/mrubyc-ncs-sample/zephyr/zephyr.exe

# QEMU（起動とタスク動作は確認できるが、タイマーが実時間より速く進む）
west build -b qemu_cortex_m3 mrubyc-ncs-sample -t run
```

## 検証状況

NCS v3.4.2 での `nrf54l15dk/nrf54l15/cpuapp` 向けビルドと、QEMU・native_sim での 2 タスクの動作は確認済みです。nRF54L15-DK の実機では、本リポジトリでは試していません。

## ライセンス

BSD-3-Clause（`LICENSE` を参照）
