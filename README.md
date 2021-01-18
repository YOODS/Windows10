# Windows10用インストーラ, マニュアルなど
##ご質問等
このリポジトリのissueを参照してください。類似の質問・課題がない場合は新規のissueを立上げてください。

# ソフトウェア/SDKのバージョンについて
## version 1.0 
	- 2020/02まで出荷分
	- calibフォルダに手動でキャリブファイルをコピーして使ってください。
	- calibファイルは納入時にご連絡したURLからダウンロードしてください。

## version 1.1
	- 2020/03からの出荷分
	- カメラキャリブデーターはカメラ内部に書き込まれていて、ソフトウェア起動時に自動的にcalibフォルダにダウンロードされます。
		- 従来の方式(カメラキャリブファイル配置)も可能
	- 位相シフト計算処理高速化
	- キャリブボード認識機能改修

## version 2.0
	- 2020/12/03時点の最新機能を搭載
	    - YCAM3D高速撮影版に対応
	    - 3位相方式の位相シフトに対応
	    - 点群のノイズ軽減、補間機能修正

## version 2.1
	- test_ycam起動時に表示されるYCAM3Dのファームウェアバージョンを16進数表記へ

# version 2.2
	- HDR機能を実装。