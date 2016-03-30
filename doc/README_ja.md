OpenToonz Plugin Utility
============

[English](../README.md)

OpenToonz では、COM-like な低レベルインタフェースを利用することで、FX Schematic で動作するラスターエフェクトを開発できます。
`opentoonz_plugin_utility` は、その低レベルインタフェースのシンプルなラッパーライブラリです。
本ライブラリを使用すると、あたらしいエフェクトを簡単に作成できます。

## プラグインのインストール方法

### ビルド

プラグインをソースからビルドする方法を紹介します。すでにプラグイン (拡張子 `.plugin`) がある場合は、この節はとばしてください。

利用したいプラグインのソースコードをダウンロードしてきてください。ソースコードに添付のドキュメント (たとえば `README.md`) を参照して、その指示に従ってプラグインをビルドしてください。Windows 環境ですと [`Visual Studio`](https://www.visualstudio.com/products/free-developer-offers-vs.aspx)、Mac や Linux ですと `gcc` が必要になるでしょう。また、ビルドツールとして [`CMake`](https://cmake.org/) が求められることもあります。

最後に、生成された動的リンクライブラリの拡張子 (`.dll`、`.dylib` あるいは `.so`) を `.plugin` に変更します。ビルド設定として `.plugin` が生成されるように設定されている場合は、この手順は不要です。

### インストール

生成されたプラグインをインストールする方法を説明します。
ここでは便宜的に `${path-to-opentoonz-stuff}` を OpenToonz の Stuff ディレクトリとします。
OSX では `/Applications/OpenToonz/OpenToonz_1.0_stuff/plugins/` が、Windows では `C:\OpenToonz 1.0 stuff\plugins` がデフォルト位置です。

プラグインのファイル `.plugin` を `${path-to-opentoonz-stuff}/plugins` 以下に配置します。
OpenToonz を再起動させるとプラグインが読み込まれ、FX Schematic のポップアップメニューに表示されるようになります。

## ライブラリを利用したプラグイン開発

[こちら](./doc/opentoonz_plugin_utility_ja.md) を参照してください。
