OpenToonz Plugin Utility
===

`opentoonz_plugin_utility` は OpenToonz のプラグイン開発を助ける OpenCV ベースのユーティリティ群です。フィルタやコンポジット系のエフェクトを迅速に開発できるように設計されています。

## ディレクトリ構成

* doc
 * ドキュメントです
* include
 * ライブラリのヘッダファイルです
* lib
 * 生成されたライブラリの出力ディレクトリです
* samples
 * 簡易的なサンプルエフェクトの実装が 3 つ含まれています
* src
 * ライブラリのソースです
* opentoonz_plugin_headers
 * プラグイン開発の低レベルインタフェースを定義したヘッダ群です

## ライブラリのビルド

まず、`git clone --recursive` で必要なモジュールを一括で取得してください。

`opentoonz_plugin_utility` はビルドシステムとして cmake を利用しています。また、画像処理部分について OpenCV を利用しています。これらは、各々の環境に合わせてインストールしておいてください。Windows 環境では OpenCV 3.1 (opencv_world310.lib) の利用を想定しています。

cmake を利用する通常の手順でライブラリをビルドできます。環境に合わせて `OpenCVConfig.cmake` があるディレクトリを、`OpenCV_DIR` 変数に設定してください。

ビルドに成功すると `lib` ディレクトリ以下にビルド設定ごとの `libopentoonz_plugin_utility.lib` が生成されます。

## サンプルエフェクト

ここでは、サンプルエフェクト `amp`, `blur`, `snp` を例に、`opentoonz_plugin_utility` を利用したエフェクト開発手順を紹介します。

### amp

入力画像の明るさを調整する amplifier エフェクトです。

入力として 1 つの画像を受け取り、`[0, 1]` の範囲をとるパラメータ `gain` を乗算した結果を出力します。

#### コードリーディング

では `samples/amp/src/main.cpp` を見ていきましょう。

```cpp
#define TNZU_DEFINE_INTERFACE
#include <toonz_utility.hpp>
```

`toonz_utility.hpp` が `opentoonz_plugin_utility` のヘッダファイルです。エフェクトのいずれか 1 つの cpp ファイルで `toonz_utility.hpp` をインクルードする前に、マクロ `TNZU_DEFINE_INTERFACE` を定義する必要があります。これによってホスト側から参照されるプラグインのインタフェースが定義されます。


```cpp
class MyFx : public tnzu::Fx {
```

エフェクトクラスの定義です。`opentoonz_plugin_utility` では、`tnzu::Fx` の派生クラスとしてエフェクトを定義します。このクラスのインスタンスが、`OpenToonz` 本体上の 1 つのエフェクトに対応します。

```cpp
enum {
  PORT_INPUT,
  PORT_COUNT,
};

int port_count() const override { return PORT_COUNT; }

char const* port_name(int i) const override {
  static std::array<char const*, PORT_COUNT> names = {
      "Input",
  };
  return names[i];
}
```

入力ポートの定義です。固定数の入力を受け付けられます。`port_name(int i)` では、`i` 番目の入力ポートの名前を返すようにします。

```cpp
enum {
  PARAM_GROUP_DEFAULT,
  PARAM_GROUP_COUNT,
};

int param_group_count() const override { return PARAM_GROUP_COUNT; }

char const* param_group_name(int i) const override {
  static std::array<char const*, PARAM_GROUP_COUNT> names = {
      "Default",
  };
  return names[i];
}
```

パラメータグループの定義です。パラメータグループとは、`OpenToonz` 本体上の表示で、パラメータをグループ分けする単位です。表示だけに影響します。`param_group_name(int i)` では、`i` 番目のパラメータグループの名前を返すようにします。

```cpp
enum {
  PARAM_GAIN,
  PARAM_COUNT,
};

int param_count() const override { return PARAM_COUNT; }

ParamPrototype const* param_prototype(int i) const override {
  static std::array<ParamPrototype, PARAM_COUNT> const params = {
      ParamPrototype{"gain", PARAM_GROUP_DEFAULT, 1, 0, 1},
  };
  return &params[i];
}
```

パラメータの定義です。固定数の `double` 型パラメータを扱えます。`param_prototype(int i)` では、`i` 番目のパラメータのプロトタイプ `ParamPrototype` を返すようにします。`ParamPrototype` は

```cpp
struct ParamPrototype {
  char const* name;
  int const group;
  double const defvalue;
  double const minvalue;
  double const maxvalue;
};
```

と定義されており、パラメータ名 `name`、パラメータグループ ID `group`、デフォルト値 `defvalue`、最小値 `minvalue`、最大値 `maxvalue` を指定します。

```cpp
int compute(Config const& config, Params const& params, Args const& args,
            cv::Mat& retimg) override try {
  DEBUG_PRINT(__FUNCTION__);

  if (args.invalid(PORT_INPUT)) {
    return 0;
  }

  double const gain = params.get<double>(PARAM_GAIN);

  retimg = args.get(PORT_INPUT) * gain;

  return 0;
} catch (cv::Exception const& e) {
  DEBUG_PRINT(e.what());
}
```

エフェクトの計算処理本体の定義です。`config` に環境情報が、`params` に上記で指定したパラメータが、`args` に上記で指定したポートからの入力画像が渡されます。計算した結果を `retimg` に代入して返します。

`args.invalid(int i)` で `i` 番目の入力がないかどうかを`bool` 値で取得できます (`args.valid(PORT_INPUT)` で、あるかどうかも取得できます)。また、`args.get(int i)` で `i` 番目の入力画像の `cv::Mat` を取得できます。フォーマットは、`OpenToonz` 本体の設定に合わせて `CV_8UC4` か `CV_16UC4` のいずれかです。

`params.get<T>(int i)` で `i` 番目のパラメータを `T` 型として取得できます。`T` には `int`、`float`、`double`、`bool` 指定できます。また、`param.get<T>(int i, double s)` を利用すると、パラメータを `s` 倍した結果を `T` 型として取得できます。同様に、`params.radian<T>(int i)` では `M_PI/180` 倍された値を取得できます。つまり、パラメータを角度の範囲 `[0, 360]` で定義しておくと、ラジアン値として取得できます。さらに、パラメータを範囲 `[0, 1]` で定義しておくと、`params.seed<T>(int i)` で乱数のシード `cv::theRNG().state` に使える値を、`params.rng<T>(int i)` でシードを設定した `std::mt19937_64` を取得できます (このとき `T=std::uint94_t` を指定することを推奨します)。

`retimg` には、入力画像とおなじフォーマットかつ、すべての入力を包含するサイズの 0 クリアされた画像が渡されます。`args.offset(int)` によって、各入力画像の `retimg` に対する相対位置 `cv::Point2d` を取得できます。また、`args.size(i)` で `args.get(i).size()` を、 `args.rect(i)` で `cv::Rect(args.offset(i), args.size(i))` を取得できます。全画面エフェクト (後述) 以外では、相対座標の値は常に非負で、入力画像サイズは `retimg` のサイズに収まります。つまり `args.get(i).copyTo(retimg(args.rect(i)))` が常に合法になっています。

**`enlarge(...)` や `compute(...)` が、`params` や `args` を変化させつつ、複数のスレッドから非同期に呼び出され得ることに注意してください。**

以上でエフェクトクラスの定義は完了です。

```cpp
namespace tnzu {
PluginInfo const* plugin_info() {
  static PluginInfo const info(TNZU_PP_STR(PLUGIN_NAME),    // name
                               TNZU_PP_STR(PLUGIN_VENDOR),  // vendor
                               "",                          // note
                               "http://dwango.co.jp/");     // helpurl
  return &info;
}

Fx* make_fx() { return new MyFx(); }
}
```

最後にライブラリ側に定義したプラグインの情報を渡す関数を定義します。

`plugin_info()` でプラグインの情報 `PluginInfo` を返します。`PluginInfo` は

```cpp
struct PluginInfo {
  std::string const name;
  std::string const vendor;
  std::string const identifier;
  std::string const note;
  std::string const helpurl;

  inline PluginInfo(std::string _name, std::string _vendor, std::string _note,
                    std::string _helpurl)
      : name(_name),
        vendor(_vendor),
        identifier(_vendor + "_" + _name),
        note(_note),
        helpurl(_helpurl) {}
};
```

と定義されており、プラグイン名 `name`、開発ベンダー名 `vendor`、簡単な説明 `note`、ヘルプ URL `helpurl` を指定します。
プラグイン名と開発ベンダー名には、アルファベットか数字ではじまり、アルファベット、数字、`_`、 で構成される任意の文字列 `[a-zA-Z][a-zA-Z0-9\_]*` を指定できます。
ただし、名前と開発ベンダー名の組み合わせが、エフェクト単位でユニークな文字列になるように注意してください。
サンプルでは `PLUGIN_NAME=OpenCV_Blur`、`PLUGIN_VENDOR=DWANGO` となるマクロを指定しています。

`make_fx()` で定義した `MyFx` のインスタンスを生成して返しています。

### blur

入力画像をガウシアンブラーでぼかす blur エフェクトです。

入力として 1 つの画像を受け取り、パラメータ `ksize-width`、`ksize-height`、`sigmaX`、`sigmaY` によるガウシアンブラーを適用したけっかを返します。

#### コードリーディング

`samples/blur/src/main.cpp` を見ていきましょう。

大部分が `amp` と共通しています。

```cpp
#include <opencv2/imgproc/imgproc.hpp>
```

`cv::GaussianBlur` を利用するためにインクルードしています。

```cpp
enum {
PARAM_KSIZE_WIDTH,
PARAM_KSIZE_HEIGHT,
PARAM_SIGMA_X,
PARAM_SIGMA_Y,
PARAM_COUNT,
};

int param_count() const override { return PARAM_COUNT; }

ParamPrototype const* param_prototype(int i) const override {
static std::array<ParamPrototype, PARAM_COUNT> const params = {
    ParamPrototype{"ksize-width", PARAM_GROUP_DEFAULT, 50, 0, 100},
    ParamPrototype{"ksize-height", PARAM_GROUP_DEFAULT, 50, 0, 100},
    ParamPrototype{"sigmaX", PARAM_GROUP_DEFAULT, 0, 0, 100},
    ParamPrototype{"sigmaY", PARAM_GROUP_DEFAULT, 0, 0, 100},
};
return &params[i];
}
```

パラメータの定義です。

```cpp
int enlarge(Config const& config, Params const& params,
            cv::Rect2d& retrc) override {
  DEBUG_PRINT(__FUNCTION__);
  cv::Size const ksize(params.get<int>(PARAM_KSIZE_WIDTH) * 2 + 1,
                       params.get<int>(PARAM_KSIZE_HEIGHT) * 2 + 1);
  retrc.x -= ksize.width / 2;
  retrc.y -= ksize.height / 2;
  retrc.width += ksize.width;
  retrc.height += ksize.height;
  return 0;
}
```

`amp` では定義されなかった関数です。エフェクトが適用される範囲を定義します。`retrc`には、すべての入力を包含する矩形サイズが指定されて渡されます。ここでは、フィルタカーネルのサイズ分のマージンが必要になるので、そのぶんだけ `retrc` を膨らませています。

```cpp
int compute(Config const& config, Params const& params, Args const& args,
            cv::Mat& retimg) override try {
  DEBUG_PRINT(__FUNCTION__);

  if (args.invalid(PORT_INPUT)) {
    return 0;
  }

  cv::Size const ksize(params.get<int>(PARAM_KSIZE_WIDTH) * 2 + 1,
                       params.get<int>(PARAM_KSIZE_HEIGHT) * 2 + 1);

  double const sigmaX = params.get<double>(PARAM_SIGMA_X);
  double const sigmaY = params.get<double>(PARAM_SIGMA_Y);

  args.get(PORT_INPUT).copyTo(retimg(args.rect(PORT_INPUT)));
  cv::GaussianBlur(retimg, retimg, ksize, sigmaX, sigmaY);

  return 0;
} catch (cv::Exception const& e) {
  DEBUG_PRINT(e.what());
}
```

フィルタ処理の定義です。入力を出力にコピーして、ガウシアンブラーを掛けているだけです。

### snp

透過率以外を反転するごま塩ノイズ (snp; salt-and-pepper) をのせるフルスクリーンエフェクトです。

入力として 1 つの画像を受け取り、パラメータ `p` の割合でノイズをのせます。疑似乱数は、パラメータ `seed` を乱数のシードとして初期化されます。

#### コードリーディング

`samples/snp/src/main.cpp` を見ていきましょう。

`blur` と同様に、大部分が `amp` と共通しています。

```cpp
enum {
  PARAM_P,
  PARAM_SEED,
  PARAM_COUNT,
};

int param_count() const override { return PARAM_COUNT; }

ParamPrototype const* param_prototype(int i) const override {
  static std::array<ParamPrototype, PARAM_COUNT> const params = {
      ParamPrototype{"p", PARAM_GROUP_DEFAULT, 0.5, 0, 1},
      ParamPrototype{"seed", PARAM_GROUP_DEFAULT, 0.5, 0, 1},
  };
  return &params[i];
}
```

パラメータの定義です。

```cpp
int enlarge(Config const& config, Params const& params,
            cv::Rect2d& retrc) override {
  DEBUG_PRINT(__FUNCTION__);
  retrc = tnzu::make_infinite_rect<double>();
  return 0;
}
```

エフェクト適用範囲の定義です。`tnzu::make_infinite_rect<double>()` により無限大サイズの `cv::Rect2d` を生成して、`retrc` に代入することで、全画面に適用するエフェクトであることを明示しています。

```cpp
int compute(Config const& config, Params const& params, Args const& args,
            cv::Mat& retimg) override try {
  DEBUG_PRINT(__FUNCTION__);

  if (args.invalid(PORT_INPUT)) {
    return 0;
  }

  double const p = params.get<double>(PARAM_P);
  std::mt19937_64 rng = params.rng<std::uint64_t>(PARAM_SEED);

  tnzu::draw_image(retimg, args.get(PORT_INPUT), args.offset(PORT_INPUT));

  if (retimg.type() == CV_8UC4) {
    return kernel<cv::Vec4b>(p, retimg, rng);
  }
  else {
    return kernel<cv::Vec4w>(p, retimg, rng);
  }
} catch (cv::Exception const& e) {
  DEBUG_PRINT(e.what());
}
```

エフェクト処理部分の定義です。ピクセルに直接さわる操作が含まれるため、`cv::Mat` の内部型にあわせて、実際の処理を `kernel<Vec4T>(...)` に委譲しています。

ここで、`retimg` のサイズが `args` のすべてを内包できるほど大きくないことに注意してください。全画面エフェクトで確保される `retimg` のサイズは、画面のサイズが最大値になります。つまり、入力画像の配置によって画面からはみ出していることがあります。そこで、ここでは `tnzu::draw_image(...)` によって入力画像を出力画像にコピーしています。


```cpp
template <typename Vec4T, typename RNG>
int MyFx::kernel(double const p, cv::Mat& retimg, RNG& rng) {
  using value_type = typename Vec4T::value_type;

  std::bernoulli_distribution rbern(p);

  for (int y = 0; y < retimg.rows; ++y) {
    Vec4T* scanline = retimg.ptr<Vec4T>(y);
    for (int x = 0; x < retimg.cols; ++x) {
      if (rbern(rng)) {
        int const alpha = scanline[x][3];
        for (int c = 0; c < 3; ++c) {
          // assume premultiplied alpha
          scanline[x][c] =
              cv::saturate_cast<value_type>(alpha - scanline[x][c]);
        }
      }
    }
  }

  return 0;
}
```

実際にノイズを載せる処理の部分です。特筆すべき点はありません。
