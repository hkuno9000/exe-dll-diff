
## はじめに ##
exediffは、Win32実行ファイル(exe/dll)のロードイメージを比較し、差異を表示するコンソールアプリケーションです。 二つのプログラムファイルの、同一性や違いを確認するのに便利です。
## 特徴 ##
  * ロードイメージのヘッダ構造を認識し、構造単位での比較を行います。
  * ロードイメージのセクションデータ(RAWDATA)の比較では、差異が一定量に達したら比較を打ち切ります。
  * ディレクトリ間で複数ファイルの比較ができます。
  * ロードイメージに埋め込まれたタイムスタンプ部分を除外して比較できます。
  * 比較ファイルのテキスト形式ダンプ(dumpbin /all 相当)を出力できます。
## インストール方法 ##
配布ファイル windiff.exe を、PATHが通ったフォルダにコピーしてください。 アンインストールするには、そのコピーしたファイルを削除してください。
## 使い方 ##
```
usage :exediff [-h?tcdq][-n#] (FILE1 FILE2 | DIR1 DIR2 [WILD] | DIR1 DIR2¥WILD)
  version 1.12
  -h -?   this help
  -t      ignore time stamp
  -c      ignore check sum
  -d      dump file image
  -q      quiet mode
  -n#     max length of differ rawdatas. default is 4
  FILE1/2 compare exe/dll file
  DIR1/2  compare folder
  WILD    compare files pattern in DIR2. default is *
```