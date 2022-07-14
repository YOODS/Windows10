@echo off
:Usage: Calib3D [-hcodb] [-f cfgfile] [-r number] [-p pattern] calib_dir
:  calib_dir    キャリブレーション画像格納先ディレクトリ
:               左右画像が分離して格納されている場合、0枚目左画像, 0枚目右画像, 1枚目左画像...の順になるようファイル名を調整してください.
:  -h           使い方を表示.
:  -c           左右連結画像を入力として与える.
:  -o           マーカ検出結果をキャリブディレクトリに保存.(入力画像と同じ名前で拡張子をpngに置き換えた形式で保存します.)
:  -d           マーカ検出結果を画面に表示する.
:  -r number    参照フレーム番号(デフォルト=0).このオプションを指定するとボード座標系に変換する(Hmatに対してのみ)
:  -f cfgfile   設定ファイル名.(デフォルト=./calib.cfg)
:  -p pattern   入力画像ファイル名パターン(*.pgm)
:
Calib3D -f .\calib.cfg -co -p pgm ..\calib
