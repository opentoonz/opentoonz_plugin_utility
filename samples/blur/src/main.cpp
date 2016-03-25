#define TNZU_DEFINE_INTERFACE
#include <toonz_utility.hpp>
#include <opencv2/imgproc/imgproc.hpp>

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
    PARAM_KSIZE_WIDTH,
    PARAM_KSIZE_HEIGHT,
    PARAM_SIGMA_X,
    PARAM_SIGMA_Y,
    PARAM_COUNT,
  };

  int param_count() const override { return PARAM_COUNT; }

  ParamPrototype const* param_prototype(int i) const override {
    static std::array<ParamPrototype, PARAM_COUNT> const params = {
        ParamPrototype{"ksize_width", PARAM_GROUP_DEFAULT, 50, 0, 100},
        ParamPrototype{"ksize_height", PARAM_GROUP_DEFAULT, 50, 0, 100},
        ParamPrototype{"sigmaX", PARAM_GROUP_DEFAULT, 0, 0, 100},
        ParamPrototype{"sigmaY", PARAM_GROUP_DEFAULT, 0, 0, 100},
    };
    return &params[i];
  }

 public:
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
};

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
