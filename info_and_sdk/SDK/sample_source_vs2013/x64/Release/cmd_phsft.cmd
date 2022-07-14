@echo off
:Usage: Phsft3D [-hrt] [-f cfgfile] [-c calib_dir] [-s images_dir] [-p pattern]
:               [-o 3d_file]
:  -h       使い方を表示
:  -r       レクティファイを実行
:  -t       点群をボード座標系に変換.(但し、boardRT.yamlがキャリブレーションディレクトリに存在する場合のみ有効. Hmatで座標系が変換されている場合、再度移動することになるので、指定しないようにしてください.
:  -f cfgfile       設定ファイル名.(デフォルト=./pshft.cfg)
:  -c calib_dir     キャリブレーションディレクトリ(.\)
:  -s images_dir    入力画像ディレクトリ(.\)
:  -p pattern       入力画像ファイル名パターン(*.pgm)
:  -o 3d_file       出力点群ファイル(phsft.ply)
:
Phsft3D.exe -r -f PHSFT.ini -c ..\calib -s ..\capt -p *.pgm -o ..\capt\out.ply
