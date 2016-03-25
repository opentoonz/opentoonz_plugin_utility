#define TNZU_DEFINE_INTERFACE
#include <toonz_utility.hpp>

class MyFx : public tnzu::Fx {
 public:
  //
  // PORT
  //
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

  //
  // PARAM GROUP
  //
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

  //
  // PARAM
  //
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

 public:
  int enlarge(Config const& config, Params const& params,
              cv::Rect2d& retrc) override {
    DEBUG_PRINT(__FUNCTION__);
    retrc = tnzu::make_infinite_rect<double>();
    return 0;
  }

  template <typename Vec4T, typename RNG>
  int kernel(double const p, cv::Mat& retimg, RNG& rng);

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
    } else {
      return kernel<cv::Vec4w>(p, retimg, rng);
    }
  } catch (cv::Exception const& e) {
    DEBUG_PRINT(e.what());
  }
};

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
