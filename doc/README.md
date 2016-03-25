OpenToonz Plugin
===

OpenToonz では、`COM-like` なインタフェースを利用することで、FX Schematic で動作するラスターエフェクトを作成できます。ここでは、作成されたプラグインエフェクトを実際に使用する手順を紹介します。

## How To Install Plugins

### Build

プラグインをソースからビルドする方法を紹介します。すでにプラグイン (拡張子 `.plugin`) がある場合は、この節はとばしてください。

利用したいプラグインのソースコードをダウンロードしてきてください。ソースコードに添付のドキュメント (たとえば `README.md`) を参照して、その指示に従ってプラグインをビルドしてください。Windows 環境ですと [`Visual Studio`](https://www.visualstudio.com/products/free-developer-offers-vs.aspx)、Mac や Linux ですと `gcc` が必要になるでしょう。また、ビルドツールとして [`CMake`](https://cmake.org/) が求められることもあります。

最後に、生成された動的リンクライブラリの拡張子 (`.dll`、`.dylib` あるいは `.so`) を `.plugin` に変更します。ビルド設定として `.plugin` が生成されるように設定されている場合は、この手順は不要です。

### Install

生成されたプラグインをインストールする方法を説明します。ここでは便宜的に `${TOONZ_STUFF}` を Toonz のディレクトリとします。

プラグインのファイル `.plugin` を `${TOONZ_STUFF}/plugins` 以下に配置します。Toonz を再起動させるとプラグインが読み込まれ、FX Schematic のポップアップメニューに表示されるようになります。

## ToonzUtility

プラグインを簡便に開発するためのラッパーライブラリです。

詳細は、[ToonzUtility](./ToonzUtility.md) を参照してください。
