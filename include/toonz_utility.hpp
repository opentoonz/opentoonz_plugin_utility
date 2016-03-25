#pragma once

#ifndef TOONZ_UTILITY_HPP_INCLUDED
#define TOONZ_UTILITY_HPP_INCLUDED

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

#include <cmath>
#include <cstdint>
#include <memory>
#include <array>
#include <limits>
#include <random>
#include <thread>
#include <sstream>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <toonz_plugin.h>
#include <toonz_hostif.h>

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
//	Visual Studio is not supported __restrict for reference
#define RESTRICT
#else
#define EXPORT
#define RESTRICT __restrict
#endif

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#define DEBUG_PRINT(MSG)                                        \
  do {                                                          \
    std::ostringstream out;                                     \
    out << "[" << tnzu::plugin_info()->name << " ("             \
        << std::this_thread::get_id() << ")]: " << MSG << '\n'; \
    OutputDebugStringA(out.str().c_str());                      \
} while (0)
#else
#define DEBUG_PRINT(MSG)     \
  do {                       \
    std::ostringstream out;  \
    out << MSG;              \
    puts(out.str().c_str()); \
} while (0)
#endif

#define TNZU_PP_STR_(X) #X
#define TNZU_PP_STR(X) TNZU_PP_STR_(X)

namespace tnzu {
template <typename T>
struct opencv_type_traits;

template <>
struct opencv_type_traits<float> {
  static int const value = CV_32FC1;
};

template <>
struct opencv_type_traits<cv::Vec2f> {
  static int const value = CV_32FC2;
};

template <>
struct opencv_type_traits<cv::Vec3f> {
  static int const value = CV_32FC3;
};

template <>
struct opencv_type_traits<cv::Vec4f> {
  static int const value = CV_32FC4;
};

template <>
struct opencv_type_traits<double> {
  static int const value = CV_64FC1;
};

template <>
struct opencv_type_traits<cv::Vec2d> {
  static int const value = CV_64FC2;
};

template <>
struct opencv_type_traits<cv::Vec3d> {
  static int const value = CV_64FC3;
};

template <>
struct opencv_type_traits<cv::Vec4d> {
  static int const value = CV_64FC4;
};
}

namespace tnzu {
// convert sRGB color space to power space
template <typename T>
inline T to_linear_color_space(T nonlinear_color, T exposure, T gamma) {
  return -std::log(T(1) - std::pow(nonlinear_color, gamma)) / exposure;
}

// convert power space to sRGB color space
template <typename T>
inline T to_nonlinear_color_space(T linear_color, T exposure, T gamma) {
  return std::pow(T(1) - std::exp(-exposure * linear_color), T(1) / gamma);
}

template <std::size_t BitDepth, typename T = float>
class linear_color_space_converter {
 public:
  using this_type = linear_color_space_converter<BitDepth, T>;
  static std::size_t const Size = 1 << BitDepth;

 public:
  inline linear_color_space_converter(T exposure, T gamma)
      : table_(new T[Size]) {
    T const scale = T(1) / Size;
    for (int i = 0; i < Size; i++) {
      table_[i] =
          tnzu::to_linear_color_space((i + T(0.5)) * scale, exposure, gamma);
    }
  }

  inline T operator[](int value) const { return table_[value]; }

 private:
  std::unique_ptr<T[]> table_;
};

template <typename T>
inline T lerp(T const& a, T const& b, double t) {
  return a + (b - a) * t;
}

template <typename T, std::size_t Sz>
inline cv::Vec<T, Sz> lerp(cv::Vec<T, Sz> const& a, cv::Vec<T, Sz> const& b,
                           double t) {
  cv::Vec<T, Sz> c;
  for (std::size_t i = 0; i < Sz; ++i) {
    c[i] = tnzu::lerp(a[i], b[i], t);
  }
  return c;
}

template <typename T, typename S>
inline T normalize_cast(S const value) {
  return cv::saturate_cast<T>(value * std::numeric_limits<T>::max());
}

template <typename T>
inline T to_radian(T degree) {
  return degree * T(M_PI / 180);
}

template <typename T>
inline T to_degree(T radian) {
  return radian * T(180 / M_PI);
}

template <typename T>
inline T square(T x) {
  return x * x;
}

template <typename Vec4T>
inline float to_gray(Vec4T const& v) {
  using T = typename Vec4T::value_type;
  return T(0.306) * v[2] + T(0.601) * v[1] + T(0.117) * v[0];
}

template <typename Vec4T>
inline Vec4T to_xyz(Vec4T const& bgr) {
  using T = typename Vec4T::value_type;
  return Vec4T(
      T(0.6069) * bgr[2] + T(0.1735) * bgr[1] + T(0.2003) * bgr[0],   // X
      T(0.2989) * bgr[2] + T(0.5866) * bgr[1] + T(0.1145) * bgr[0],   // Y
      T(0.0000) * bgr[2] + T(0.0661) * bgr[1] + T(1.1162) * bgr[0]);  // Z
}

template <typename Vec4T>
inline Vec4T to_bgr(Vec4T const& xyz) {
  using T = typename Vec4T::value_type;
  return Vec4T(
      T(+0.0585) * xyz[0] - T(0.1187) * xyz[1] + T(0.9017) * xyz[2],   // blue
      T(-0.9844) * xyz[0] + T(1.9985) * xyz[1] - T(0.0279) * xyz[2],   // green
      T(+1.9104) * xyz[0] - T(0.5338) * xyz[1] - T(0.2891) * xyz[2]);  // red
}

template <typename T>
inline cv::Point3_<T> meet(cv::Point3_<T> const& o, cv::Point3_<T> const& d,
                           T const t) {
  return o + d * t;
}

// cf. https://www.opengl.org/sdk/docs/man/html/reflect.xhtml
template <typename T>
inline cv::Point3_<T> reflect(cv::Point3_<T> const& i,
                              cv::Point3_<T> const& n) {
  return i + n * (-2 * n.dot(i));
}

// cf. https://www.opengl.org/sdk/docs/man/html/refract.xhtml
template <typename T>
inline cv::Point3_<T> refract(cv::Point3_<T> const& i, cv::Point3_<T> const& n,
                              T const eta) {
  double const c = n.dot(i);
  double const k = 1.0 - eta * eta * (1 - c * c);
  return i * eta - n * (eta * c + sqrt(k));
}
}

namespace tnzu {
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

extern PluginInfo const* plugin_info();

class Fx {
 public:
  virtual ~Fx();

  static std::string get_stuff_dir();

 public:
  virtual int port_count() const = 0;
  virtual char const* port_name(int i) const = 0;

  virtual int param_group_count() const = 0;
  virtual char const* param_group_name(int i) const = 0;

  struct ParamPrototype {
    char const* name;
    int const group;
    double const defvalue;
    double const minvalue;
    double const maxvalue;
  };

  virtual int param_count() const = 0;
  virtual ParamPrototype const* param_prototype(int i) const = 0;

  class Params {
   public:
    inline Params(int paramc) : params_(paramc) {}

    inline double operator[](std::size_t const i) const { return params_[i]; }
    inline double& operator[](std::size_t const i) { return params_[i]; }

   public:
    template <typename T>
    inline T get(int i) const;

    template <typename T>
    inline T get(int i, double scale) const;

    template <typename T>
    inline T radian(int i) const;

    // seed for random number generators
    template <typename T>
    inline T seed(int i) const;

    // random number generator
    template <typename T>
    inline std::mt19937_64 rng(int i) const;

   private:
    std::vector<double> params_;
  };

  class Args {
   public:
    inline Args(int argc) : valid_(argc, false), args_(argc), offsets_(argc) {}

    inline void set(std::size_t i, cv::Mat arg, cv::Point2d offset) {
      valid_[i] = true;
      args_[i] = arg;
      offsets_[i] = offset;
    }

   public:
    int count() const { return static_cast<int>(args_.size()); }

    inline bool valid(std::size_t i) const { return valid_[i]; }
    inline bool invalid(std::size_t i) const { return !valid_[i]; }

    inline cv::Mat const& get(std::size_t i) const { return args_[i]; }

    inline cv::Point2d offset(std::size_t i) const { return offsets_[i]; }

    inline cv::Size2d size(std::size_t i) const { return args_[i].size(); }

    inline cv::Rect2d rect(std::size_t i) const {
      return cv::Rect2d(offset(i), size(i));
    }

    inline cv::Point2d& offset(std::size_t i) { return offsets_[i]; }

   private:
    std::vector<bool> valid_;
    std::vector<cv::Mat> args_;
    std::vector<cv::Point2d> offsets_;
  };

  // cf. toonz::rendering_setting_t
  struct Config {
    toonz_affine_t affine;
    double gamma;
    double time_stretch_from;
    double time_stretch_to;
    double stereo_scopic_shift;
    int bpp;
    int max_tile_size;
    int quality;
    int field_prevalence;
    int stereoscopic;
    int is_swatch;
    int user_cachable;
    int apply_shrink_to_viewer;

    int frame;
  };

 public:
  virtual int init();

  virtual int begin_render();
  virtual int end_render();

  virtual int begin_frame();
  virtual int end_frame();

  virtual int enlarge(Config const& config, Params const& params,
                      cv::Rect2d& retrc);
  virtual int compute(Config const& config, Params const& params,
                      Args const& args, cv::Mat& retimg) = 0;

 public:
  inline toonz::node_handle_t handle() const { return handle_; }
  inline toonz::node_handle_t& handle() { return handle_; }

 public:
  toonz::node_handle_t handle_;
};

template <typename T>
inline T Fx::Params::get(int i) const {
  return static_cast<T>(params_[i]);
}

template <>
inline int Fx::Params::get(int i) const {
  return static_cast<int>(std::round(params_[i]));
}

template <>
inline bool Fx::Params::get(int i) const {
  return params_[i] >= 0.5;
}

template <typename T>
inline T Fx::Params::get(int i, double scale) const {
  return static_cast<T>(params_[i] * scale);
}

template <>
inline int Fx::Params::get(int i, double scale) const {
  return static_cast<int>(std::round(params_[i] * scale));
}

template <typename T>
inline T Fx::Params::radian(int i) const {
  return static_cast<T>(tnzu::to_radian<double>(params_[i]));
}

// seed for random number generators
template <typename T>
inline T Fx::Params::seed(int i) const {
  return static_cast<T>(params_[i] * std::numeric_limits<T>::max());
}

// random number generator
template <typename T>
inline std::mt19937_64 Fx::Params::rng(int i) const {
  return std::mt19937_64(seed<T>(i));
}

extern Fx* make_fx();
}

namespace tnzu {
inline std::size_t bit_reverse(std::size_t x) {
  x = ((x & 0x5555555555555555LLU) << 1) | ((x >> 1) & 0x5555555555555555LLU);
  x = ((x & 0x3333333333333333LLU) << 2) | ((x >> 2) & 0x3333333333333333LLU);
  x = ((x & 0x0f0f0f0f0f0f0f0fLLU) << 4) | ((x >> 4) & 0x0f0f0f0f0f0f0f0fLLU);
  x = ((x & 0x00ff00ff00ff00ffLLU) << 8) | ((x >> 8) & 0x00ff00ff00ff00ffLLU);
  x = ((x & 0x0000ffff0000ffffLLU) << 16) | ((x >> 16) & 0x0000ffff0000ffffLLU);
  x = ((x & 0x00000000ffffffffLLU) << 32) | ((x >> 32) & 0x00000000ffffffffLLU);
  return x;
}

inline bool file_exists(char const* file_name) {
  if (FILE* fp = fopen(file_name, "r")) {
    fclose(fp);
    return true;
  } else {
    return false;
  }
}

// hash code
std::size_t hash(cv::Mat const& m);

// snp (salt and pepper) noise
template <typename VecT>
cv::Mat make_snp_noise(cv::Size const size, float const low, float const high) {
  cv::Mat retval = cv::Mat::zeros(size, tnzu::opencv_type_traits<VecT>::value);
  cv::randu(retval, low, high);
  return retval;
}

// Perlin noise
template <typename VecT, std::size_t Sz>
cv::Mat make_perlin_noise(cv::Size const size,
                          std::array<float, Sz> const& amp) {
  cv::Mat retval = cv::Mat::zeros(size, tnzu::opencv_type_traits<VecT>::value);

  int num = 0;
  for (std::size_t i = 0; i < Sz; ++i) {
    float const range = amp[i];
    cv::Size const octave_size(2 << i, 2 << i);

    cv::Mat field = tnzu::make_snp_noise<VecT>(octave_size, -range, range);

    float const scale = std::max(float(size.width) / octave_size.width,
                                 float(size.height) / octave_size.height);
    cv::resize(field, field, size, scale, scale, cv::INTER_CUBIC);
    retval += field;
  }

  return retval;
}

void generate_bloom(cv::Mat& img, int level, int radius = 1);

template <typename T>
cv::Rect_<T> make_infinite_rect() {
  return cv::Rect_<T>(
      -std::numeric_limits<T>::infinity(), -std::numeric_limits<T>::infinity(),
      +std::numeric_limits<T>::infinity(), +std::numeric_limits<T>::infinity());
}

void draw_image(cv::Mat& canvas, cv::Mat const& img, cv::Point2d pos);

template <typename Vec4T>
Vec4T tap_texel(cv::Mat const& src, cv::Point2d const& pos) {
  int const x0 = static_cast<int>(std::floor(pos.x));
  int const y0 = static_cast<int>(std::floor(pos.y));
  int const x1 = cv::borderInterpolate(x0 + 1, src.cols, cv::BORDER_WRAP);
  int const y1 = cv::borderInterpolate(y0 + 1, src.rows, cv::BORDER_WRAP);
  float const sx = pos.x - x0;
  float const sy = pos.y - y0;

  Vec4T const s00 = src.at<Vec4T>(y0, x0);
  Vec4T const s01 = src.at<Vec4T>(y0, x1);
  Vec4T const s10 = src.at<Vec4T>(y1, x0);
  Vec4T const s11 = src.at<Vec4T>(y1, x1);

  return tnzu::lerp(tnzu::lerp(s00, s01, sx), tnzu::lerp(s10, s11, sx), sy);
}
}

#ifdef TNZU_DEFINE_INTERFACE
extern "C" {
EXPORT int toonz_plugin_init(toonz::host_interface_t* hostif) {
  extern int toonz_plugin_init_main(toonz::host_interface_t * hostif);
  return toonz_plugin_init_main(hostif);
}

EXPORT void toonz_plugin_exit() {
  extern void toonz_plugin_exit_main();
  toonz_plugin_exit_main();
}

extern void do_compute(toonz_node_handle_t node,
                       const toonz_rendering_setting_t*, double frame,
                       toonz_tile_handle_t tile);
extern int do_get_bbox(toonz_node_handle_t node,
                       const toonz_rendering_setting_t*, double frame,
                       toonz_rect_t* rect);
extern int can_handle(toonz_node_handle_t node,
                      const toonz_rendering_setting_t*, double frame);
extern size_t get_memory_requirement(toonz_node_handle_t node,
                                     const toonz_rendering_setting_t*,
                                     double frame, const toonz_rect_t* rect);

extern void on_new_frame(toonz_node_handle_t node,
                         const toonz_rendering_setting_t*, double frame);
extern void on_end_frame(toonz_node_handle_t node,
                         const toonz_rendering_setting_t*, double frame);

extern int node_create(toonz_node_handle_t node);
extern int node_destroy(toonz_node_handle_t node);
extern int plugin_setup(toonz_node_handle_t node);
extern int start_render(toonz_node_handle_t node);
extern int end_render(toonz_node_handle_t node);

toonz_nodal_rasterfx_handler_t_ toonz_plugin_node_handler = {
    {1, 0},
    do_compute,
    do_get_bbox,
    can_handle,
    get_memory_requirement,
    on_new_frame,
    on_end_frame,
    node_create,
    node_destroy,
    plugin_setup,
    start_render,
    end_render,
};

TOONZ_PLUGIN_PROBE_BEGIN(TOONZ_IF_VER(1, 0))
TOONZ_PLUGIN_PROBE_DEFINE(TOONZ_PLUGIN_VER(1, 0),
                          tnzu::plugin_info()->name.c_str(),
                          tnzu::plugin_info()->vendor.c_str(),
                          tnzu::plugin_info()->identifier.c_str(),
                          tnzu::plugin_info()->note.c_str(),
                          tnzu::plugin_info()->helpurl.c_str(),
                          TOONZ_PLUGIN_CLASS_POSTPROCESS_SLAB,
                          &toonz_plugin_node_handler)
TOONZ_PLUGIN_PROBE_END;
}
#endif

#endif  // TOONZ_UTILITY_HPP_INCLUDED
