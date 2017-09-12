#include <toonz_utility.hpp>

#include <cstdio>
#include <memory>
#include <cmath>
#include <vector>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <toonz_params.h>

namespace {

std::string get_system_var(char const* key, char const* value) {
#ifdef _WIN32
  HKEY hKey = nullptr;
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, NULL, KEY_QUERY_VALUE, &hKey) !=
      ERROR_SUCCESS) {
    DEBUG_PRINT("ERROR could not get a reg key");
    return ".";
  }

  DWORD dwType = 0;
  std::array<BYTE, MAX_PATH> lpData;
  DWORD dwDataSize = lpData.size();
  if (::RegQueryValueEx(hKey, value, 0, &dwType, lpData.data(), &dwDataSize) !=
      ERROR_SUCCESS) {
    DEBUG_PRINT("ERROR could not get a reg value");
  }

  return reinterpret_cast<char const*>(lpData.data());
#else
  return ".";
#endif
}

toonz_host_interface_t* ifactory = NULL;

void release_interf(void* interf) { ifactory->release_interface(interf); }

template <class T>
std::unique_ptr<T, decltype(&release_interf)> grab_interf(
    const toonz_UUID* uuid) {
  T* interf = nullptr;
  if (ifactory->query_interface(uuid, reinterpret_cast<void**>(&interf)))
    return std::unique_ptr<T, decltype(&release_interf)>(nullptr,
                                                         release_interf);
  return std::move(
      std::unique_ptr<T, decltype(&release_interf)>(interf, release_interf));
}

#define TOONZ_DEF_IF(Name, Type)                                 \
  std::unique_ptr<Type, decltype(&release_interf)> Name(nullptr, \
                                                        &release_interf)

#define TOONZ_SET_IF(Name, Type, UUID)           \
  do {                                           \
    Name = grab_interf<Type>(UUID);              \
    if (!Name) {                                 \
      DEBUG_PRINT("ERROR could not get " #Name); \
      return TOONZ_ERROR_FAILED_TO_CREATE;       \
    }                                            \
  \
} while (0)

TOONZ_DEF_IF(nodeif, toonz_node_interface_t);
TOONZ_DEF_IF(portif, toonz_port_interface_t);
TOONZ_DEF_IF(fxif, toonz_fxnode_interface_t);
TOONZ_DEF_IF(paramif, toonz_param_interface_t);
TOONZ_DEF_IF(tileif, toonz_tile_interface_t);

#undef TOONZ_DEF_IF

std::vector<std::vector<toonz_param_desc_t>> build_params(
    std::unique_ptr<tnzu::Fx> const& fx) {
  std::vector<std::vector<toonz_param_desc_t>> params(fx->param_group_count());
  for (int i = 0, nparams = fx->param_count(); i < nparams; i++) {
    int const k = fx->param_prototype(i)->group;

    toonz_param_desc_t desc = {};
    desc.base.ver.major = 1;
    desc.base.ver.minor = 0;
    desc.base.type = TOONZ_PARAM_DESC_TYPE_PARAM;
    desc.base.label = fx->param_prototype(i)->name;
    desc.key = fx->param_prototype(i)->name;
    desc.note = "";
    desc.traits_tag = TOONZ_PARAM_TYPE_DOUBLE;
    desc.traits.d.def = fx->param_prototype(i)->defvalue;
    desc.traits.d.min = fx->param_prototype(i)->minvalue;
    desc.traits.d.max = fx->param_prototype(i)->maxvalue;
    params[k].push_back(desc);
  }
  return params;
}

std::vector<toonz_param_group_t> build_groups(
    std::unique_ptr<tnzu::Fx> const& fx,
    std::vector<std::vector<toonz_param_desc_t>> const& params) {
  std::vector<toonz_param_group_t> groups;
  for (int k = 0, ngroups = fx->param_group_count(); k < ngroups; k++) {
    toonz_param_group_t group = {};
    group.base.ver.major = 1;
    group.base.ver.minor = 0;
    group.base.type = TOONZ_PARAM_DESC_TYPE_GROUP;
    group.base.label = fx->param_group_name(k);
    group.num = static_cast<int>(params[k].size());
    group.array = const_cast<toonz_param_desc_t*>(params[k].data());
    groups.push_back(group);
  }
  return groups;
}

toonz_param_page_t build_page(char const* label, std::size_t n,
                              toonz_param_group_t const* g) {
  toonz_param_page_t page;
  page.base.ver.major = 1;
  page.base.ver.minor = 0;
  page.base.type = TOONZ_PARAM_DESC_TYPE_PAGE;
  page.base.label = label;
  page.num = n;
  page.array = const_cast<toonz_param_group_t*>(g);
  return page;
}

template <typename Vec4T>
void copy_image(cv::Point src_offset, cv::Mat const& src, cv::Point dst_offset,
                cv::Mat& dst, cv::Size size, cv::Point2d t) {
  int const max_value = std::numeric_limits<typename Vec4T::value_type>::max();

  for (int y = 0; y < size.height; ++y) {
    Vec4T* dst_ptr = dst.ptr<Vec4T>(dst_offset.y + y);

    int const y0 =
        cv::borderInterpolate(src_offset.y + y + 0, src.rows, cv::BORDER_WRAP);
    int const y1 =
        cv::borderInterpolate(src_offset.y + y + 1, src.rows, cv::BORDER_WRAP);

    for (int x = 0; x < size.width; ++x) {
      Vec4T d = dst_ptr[dst_offset.x + x];

      int const x0 = cv::borderInterpolate(src_offset.x + x + 0, src.cols,
                                           cv::BORDER_WRAP);
      int const x1 = cv::borderInterpolate(src_offset.x + x + 1, src.cols,
                                           cv::BORDER_WRAP);

      Vec4T const s00 = src.at<Vec4T>(y0, x0);
      Vec4T const s01 = src.at<Vec4T>(y0, x1);
      Vec4T const s10 = src.at<Vec4T>(y1, x0);
      Vec4T const s11 = src.at<Vec4T>(y1, x1);

      Vec4T const s =
          tnzu::lerp(tnzu::lerp(s00, s01, t.x), tnzu::lerp(s10, s11, t.x), t.y);

      for (int c = 0; c < 4; ++c) {
        d[c] = d[c] * (max_value - s[3]) / max_value + s[c];
      }

      dst_ptr[dst_offset.x + x] = d;
    }
  }
}

}  //  end of unnamed namespace

namespace tnzu {
Fx::~Fx() {}

std::string Fx::get_stuff_dir() {
  static std::string const dir =
      get_system_var("SOFTWARE\\OpenToonz\\OpenToonz\\1.1", "TOONZROOT");
  return dir;
}

int Fx::init() { return 0; }

int Fx::begin_render() { return 0; }

int Fx::end_render() { return 0; }

int Fx::begin_frame() { return 0; }

int Fx::end_frame() { return 0; }

int Fx::enlarge(Config const& config, Params const& params, cv::Rect2d& retrc) {
  return 0;
}

void draw_image(cv::Mat& dst, cv::Mat const& src, cv::Point2d pos) {
  if (src.type() != dst.type()) {
    return;
  }

  if ((src.type() != CV_8UC4) && (src.type() != CV_16UC4)) {
    return;
  }

  cv::Size const dst_size = dst.size();

  cv::Point2d src_offset(0, 0);
  cv::Size src_size = src.size();

  cv::Point dst_offset(static_cast<int>(std::floor(pos.x)),
                       static_cast<int>(std::floor(pos.y)));

  if ((dst_offset.x >= dst_size.width) ||
      (dst_offset.x + src_size.width <= 0) ||
      (dst_offset.y >= dst_size.height) ||
      (dst_offset.y + src_size.height <= 0)) {
    return;
  }

  if (dst_offset.x < 0) {
    src_offset.x -= dst_offset.x;
    src_size.width += dst_offset.x;
    dst_offset.x = 0;
  }

  if (dst_offset.x + src_size.width > dst_size.width) {
    src_size.width = dst_size.width - dst_offset.x;
  }

  if (dst_offset.y < 0) {
    src_offset.y -= dst_offset.y;
    src_size.height += dst_offset.y;
    dst_offset.y = 0;
  }

  if (dst_offset.y + src_size.height > dst_size.height) {
    src_size.height = dst_size.height - dst_offset.y;
  }

  if ((src_size.width <= 0) || (src_size.height <= 0)) {
    return;
  }

  cv::Point2d t(src_offset.x - std::floor(src_offset.x),
                src_offset.y - std::floor(src_offset.y));

  if (src.type() == CV_8UC4) {
    copy_image<cv::Vec4b>(src_offset, src, dst_offset, dst, src_size, t);
  } else {
    copy_image<cv::Vec4w>(src_offset, src, dst_offset, dst, src_size, t);
  }
}

std::size_t hash(cv::Mat const& m) {
  static std::size_t const FNV_OFFSET_BASIS = 14695981039346656037LLU;
  static std::size_t const FNV_PRIME = 1099511628211LLU;
  static std::size_t const SAMPLE_SIZE = 256;

  std::size_t const size = m.cols * m.rows * m.elemSize();

  // sample by Van der Corput sequence
  std::size_t h = FNV_OFFSET_BASIS;
  for (std::size_t i = 1; i <= SAMPLE_SIZE; ++i) {
    // sample point
    double const j =
        tnzu::bit_reverse(i) / (std::numeric_limits<std::size_t>::max() + 1.0);
    std::size_t const k = static_cast<std::size_t>(j * size);

    // update hash value
    h *= FNV_PRIME;
    h ^= m.data[k];
  }
  return h;
}

void generate_bloom(cv::Mat& img, int level, int radius) {
  std::vector<cv::Mat> dst(level + 1);

  cv::Size const ksize(radius * 2 + 1, radius * 2 + 1);

  cv::Mat tmp;
  int i;
  cv::Size size = img.size();
  for (i = 0; i <= level;) {
    if (i) {
      cv::resize(img, tmp, size / 2, 0.0, 0.0, cv::INTER_AREA);
      img = tmp;
      size = img.size();
    }

    cv::GaussianBlur(img, dst[i], ksize, 0.0);

    ++i;

    if ((size.width <= 1) || (size.height <= 1)) {
      break;
    }
  }

  for (--i; i > 0; --i) {
    cv::resize(dst[i], tmp, dst[i - 1].size());
    dst[i - 1] += tmp;
  }
  img = dst[0];
}
}

//
// tnzu
//
template <typename T>
bool to_mat(toonz::tile_handle_t tile, cv::Mat& mat) {
  mat = cv::Scalar(0, 0, 0, 0);

  toonz::rect_t rect;
  tileif->get_rectangle(tile, &rect);

  cv::Size const size(static_cast<int>(rect.x1 - rect.x0),
                      static_cast<int>(rect.y1 - rect.y0));

  int stride = 0;
  tileif->get_raw_stride(tile, &stride);
  stride /= sizeof(typename T::value_type);

  typename T::value_type* data;
  tileif->get_raw_address_unsafe(tile, reinterpret_cast<void**>(&data));
  if (!data) {
    tileif->safen(tile);
    return false;
  }

  for (int y = 0; y < size.height; ++y) {
    T* dst = mat.ptr<T>(y);
    for (int x = 0; x < size.width; ++x) {
      int p = y * stride + x * 4;
      for (int c = 0; c < 4; c++) {
        dst[x][c] = data[p++];
      }
    }
  }

  tileif->safen(tile);
  return true;
}

template <typename T>
bool from_mat(toonz::tile_handle_t tile, toonz::rect_t bbox,
              cv::Mat const& mat) {
  typename T::value_type* data;
  tileif->get_raw_address_unsafe(tile, reinterpret_cast<void**>(&data));
  if (!data) {
    tileif->safen(tile);
    return false;
  }

  int stride = 0;
  tileif->get_raw_stride(tile, &stride);
  stride /= sizeof(typename T::value_type);

  toonz::rect_t rect;
  tileif->get_rectangle(tile, &rect);

  toonz::rect_t roi;
  roi.x0 = std::max(rect.x0, bbox.x0);
  roi.y0 = std::max(rect.y0, bbox.y0);
  roi.x1 = std::min(rect.x1, bbox.x1);
  roi.y1 = std::min(rect.y1, bbox.y1);

  cv::Point const src_offset(std::max(0, static_cast<int>(roi.x0 - bbox.x0)),
                             std::max(0, static_cast<int>(roi.y0 - bbox.y0)));

  cv::Point const dst_offset(std::max(0, static_cast<int>(roi.x0 - rect.x0)),
                             std::max(0, static_cast<int>(roi.y0 - rect.y0)));

  cv::Size const size(static_cast<int>(roi.x1 - roi.x0),
                      static_cast<int>(roi.y1 - roi.y0));

  for (int y = 0; y < size.height; ++y) {
    T const* src = mat.ptr<T>(y + src_offset.y);
    for (int x = 0; x < size.width; ++x) {
      int p = x + src_offset.x;
      int q = (y + dst_offset.y) * stride + (x + dst_offset.x) * 4;
      for (int c = 0; c < 4; ++c) {
        data[q++] = src[p][c];
      }
    }
  }

  tileif->safen(tile);
  return true;
}

//
// implementation
//
extern "C" {

void do_compute(toonz_node_handle_t node, const toonz_rendering_setting_t* rs,
                double frame, toonz_tile_handle_t tile) {
  tnzu::Fx* fx = nullptr;
  nodeif->get_user_data(node, reinterpret_cast<void**>(&fx));

  DEBUG_PRINT(__FUNCTION__ << " : node=" << node << ", data=" << fx);
  if (!fx) {
    return;
  }

  int elem_type = TOONZ_TILE_TYPE_32P;
  tileif->get_element_type(tile, &elem_type);
  if ((elem_type != TOONZ_TILE_TYPE_32P) &&
      (elem_type != TOONZ_TILE_TYPE_64P)) {
    DEBUG_PRINT("WARNING unsupported pixel format");
    return;
  }

  int const paramc = fx->param_count();
  tnzu::Fx::Params params(paramc);
  for (int i = 0; i < paramc; i++) {
    toonz::param_handle_t param = nullptr;
    if (int const ret =
            nodeif->get_param(node, fx->param_prototype(i)->name, &param)) {
      return;
    }

    int size_in_elements = 1;
    paramif->get_value(param, frame, &size_in_elements, &params[i]);
  }

  int const argc = fx->port_count();
  tnzu::Fx::Args args(argc);

  toonz::rect_t bbox;

  bbox.x0 = +std::numeric_limits<double>::infinity();
  bbox.y0 = +std::numeric_limits<double>::infinity();
  bbox.x1 = -std::numeric_limits<double>::infinity();
  bbox.y1 = -std::numeric_limits<double>::infinity();

  for (int i = 0; i < argc; i++) {
    toonz::port_handle_t port = nullptr;
    nodeif->get_input_port(node, fx->port_name(i), &port);
    if (!port) {
      DEBUG_PRINT("WARNING null port");
      continue;
    }

    int con = 0;
    portif->is_connected(port, &con);
    if (!con) {
      DEBUG_PRINT("WARNING disconnected port");
      continue;
    }

    toonz::fxnode_handle_t fx = nullptr;
    portif->get_fx(port, &fx);
    if (!fx) {
      DEBUG_PRINT("WARNING invalid port");
      continue;
    }

    int got = 0;
    toonz::rect_t inbbox;
    fxif->get_bbox(fx, rs, frame, &inbbox, &got);
    if (!got) {
      DEBUG_PRINT("WARNING could not get bbox");
      continue;
    }

    if ((inbbox.x0 == -std::numeric_limits<double>::max()) ||
        (inbbox.y0 == -std::numeric_limits<double>::max()) ||
        (inbbox.x1 == std::numeric_limits<double>::max()) ||
        (inbbox.y1 == std::numeric_limits<double>::max())) {
      // fullscreen effect
      DEBUG_PRINT("fullscreen");
      tileif->get_rectangle(tile, &inbbox);
    }

    toonz::tile_handle_t intile = nullptr;
    tileif->create(&intile);
    if (!intile) {
      continue;
    }
    fxif->compute_to_tile(fx, rs, frame, &inbbox, NULL, intile);

    int in_elem_type = TOONZ_TILE_TYPE_32P;
    tileif->get_element_type(intile, &in_elem_type);
    if ((elem_type != TOONZ_TILE_TYPE_32P) &&
        (elem_type != TOONZ_TILE_TYPE_64P)) {
      DEBUG_PRINT("WARNING unsupported pixel format");
      tileif->destroy(intile);
      continue;
    }

    cv::Size const insize(static_cast<int>(inbbox.x1 - inbbox.x0),
                          static_cast<int>(inbbox.y1 - inbbox.y0));

    cv::Mat mat;
    if (elem_type == TOONZ_TILE_TYPE_32P) {
      DEBUG_PRINT("INFO input elem_type = TOONZ_TILE_TYPE_32P");
      mat = cv::Mat(insize, CV_8UC4);
      if (!to_mat<cv::Vec4b>(intile, mat)) {
        tileif->destroy(intile);
        continue;
      }
    } else {
      DEBUG_PRINT("INFO input elem_type = TOONZ_TILE_TYPE_64P");
      mat = cv::Mat(insize, CV_16UC4);
      if (!to_mat<cv::Vec4w>(intile, mat)) {
        tileif->destroy(intile);
        continue;
      }
    }

    bbox.x0 = std::min(bbox.x0, inbbox.x0);
    bbox.y0 = std::min(bbox.y0, inbbox.y0);
    bbox.x1 = std::max(bbox.x1, inbbox.x1);
    bbox.y1 = std::max(bbox.y1, inbbox.y1);

    args.set(i, mat, cv::Point2d(inbbox.x0, inbbox.y0));

    tileif->destroy(intile);
  }

  tnzu::Fx::Config const cfg = {
      rs->affine, rs->gamma, rs->time_stretch_from, rs->time_stretch_to,
      rs->stereo_scopic_shift, rs->bpp, rs->max_tile_size, rs->quality,
      rs->field_prevalence, rs->stereoscopic, rs->is_swatch, rs->user_cachable,
      rs->apply_shrink_to_viewer, static_cast<int>(frame),
  };

  cv::Rect2d rect(bbox.x0, bbox.y0, bbox.x1 - bbox.x0, bbox.y1 - bbox.y0);
  fx->enlarge(cfg, params, rect);

  if ((rect.width <= 0.0) || (rect.height <= 0.0)) {
    DEBUG_PRINT("WARNING null rectangle");
    return;
  }

  if (!std::isfinite(rect.x) || !std::isfinite(rect.y) ||
      !std::isfinite(rect.width) || !std::isfinite(rect.height)) {
    // fullscreen effect

    tileif->get_rectangle(tile, &bbox);
    rect.x = bbox.x0;
    rect.y = bbox.y0;
    rect.width = bbox.x1 - bbox.x0;
    rect.height = bbox.y1 - bbox.y0;
  } else {
    bbox.x0 = rect.x;
    bbox.y0 = rect.y;
    bbox.x1 = rect.x + rect.width;
    bbox.y1 = rect.y + rect.height;
  }

  for (int i = 0, count = fx->port_count(); i < count; i++) {
    // offsets are often negatives, when using an fullscreen effect
    args.offset(i).x -= bbox.x0;
    args.offset(i).y -= bbox.y0;
  }

  cv::Mat retimg;
  if (elem_type == TOONZ_TILE_TYPE_32P) {
    retimg = cv::Mat(cv::Size(static_cast<int>(std::ceil(rect.width)),
                              static_cast<int>(std::ceil(rect.height))),
                     CV_8UC4, cv::Scalar(0, 0, 0, 0));
  } else {
    retimg = cv::Mat(cv::Size(static_cast<int>(std::ceil(rect.width)),
                              static_cast<int>(std::ceil(rect.height))),
                     CV_16UC4, cv::Scalar(0, 0, 0, 0));
  }

  fx->compute(cfg, params, args, retimg);

  if (elem_type == TOONZ_TILE_TYPE_32P) {
    DEBUG_PRINT("INFO output elem_type = TOONZ_TILE_TYPE_32P");
    if (!from_mat<cv::Vec4b>(tile, bbox, retimg)) {
      DEBUG_PRINT("WARNING fail copying to the tile");
      return;
    }
  } else {
    DEBUG_PRINT("INFO output elem_type = TOONZ_TILE_TYPE_64P");
    if (!from_mat<cv::Vec4w>(tile, bbox, retimg)) {
      DEBUG_PRINT("WARNING fail copying to the tile");
      return;
    }
  }
}

int do_get_bbox(toonz_node_handle_t node, const toonz_rendering_setting_t* rs,
                double frame, toonz_rect_t* bbox) {
  tnzu::Fx* fx = nullptr;
  nodeif->get_user_data(node, reinterpret_cast<void**>(&fx));

  DEBUG_PRINT(__FUNCTION__ << " : node=" << node << ", data=" << fx);
  if (!fx) {
    return 1;
  }

  int const paramc = fx->param_count();
  tnzu::Fx::Params params(paramc);
  for (int i = 0; i < paramc; i++) {
    toonz::param_handle_t param = nullptr;
    if (int const ret =
            nodeif->get_param(node, fx->param_prototype(i)->name, &param)) {
      return 1;
    }

    int size_in_elements = 1;
    paramif->get_value(param, frame, &size_in_elements, &params[i]);
  }

  bbox->x0 = +std::numeric_limits<double>::infinity();
  bbox->y0 = +std::numeric_limits<double>::infinity();
  bbox->x1 = -std::numeric_limits<double>::infinity();
  bbox->y1 = -std::numeric_limits<double>::infinity();

  for (int i = 0, argc = fx->port_count(); i < argc; i++) {
    toonz::port_handle_t port = nullptr;
    nodeif->get_input_port(node, fx->port_name(i), &port);
    if (!port) {
      continue;
    }

    int con = 0;
    portif->is_connected(port, &con);
    if (!con) {
      continue;
    }

    toonz::fxnode_handle_t fx = nullptr;
    portif->get_fx(port, &fx);
    if (!fx) {
      DEBUG_PRINT("WARNING could not get a fx of the port");
      continue;
    }

    int got = 0;
    toonz::rect_t inbbox;
    fxif->get_bbox(fx, rs, frame, &inbbox, &got);
    if (!got) {
      continue;
    }

    bbox->x0 = std::min(bbox->x0, inbbox.x0);
    bbox->y0 = std::min(bbox->y0, inbbox.y0);
    bbox->x1 = std::max(bbox->x1, inbbox.x1);
    bbox->y1 = std::max(bbox->y1, inbbox.y1);
  }

  tnzu::Fx::Config const cfg = {
      rs->affine, rs->gamma, rs->time_stretch_from, rs->time_stretch_to,
      rs->stereo_scopic_shift, rs->bpp, rs->max_tile_size, rs->quality,
      rs->field_prevalence, rs->stereoscopic, rs->is_swatch, rs->user_cachable,
      rs->apply_shrink_to_viewer, static_cast<int>(frame),
  };

  cv::Rect2d rect(bbox->x0, bbox->y0, bbox->x1 - bbox->x0, bbox->y1 - bbox->y0);
  fx->enlarge(cfg, params, rect);

  if ((rect.width <= 0.0) || (rect.height <= 0.0)) {
    return 1;
  }

  if (!std::isfinite(rect.x) || !std::isfinite(rect.width) ||
      !std::isfinite(rect.y) || !std::isfinite(rect.height)) {
    bbox->x0 = -std::numeric_limits<double>::max();
    bbox->y0 = -std::numeric_limits<double>::max();
    bbox->x1 = +std::numeric_limits<double>::max();
    bbox->y1 = +std::numeric_limits<double>::max();
  } else {
    bbox->x0 = rect.x;
    bbox->y0 = rect.y;
    bbox->x1 = rect.x + rect.width;
    bbox->y1 = rect.y + rect.height;
  }

  return 0;
}

int can_handle(toonz_node_handle_t node, const toonz_rendering_setting_t* rs,
               double frame) {
  DEBUG_PRINT(__FUNCTION__);
  return TOONZ_OK;
}

size_t get_memory_requirement(toonz_node_handle_t node,
                              const toonz_rendering_setting_t* rs, double frame,
                              const toonz_rect_t* rect) {
  DEBUG_PRINT(__FUNCTION__);
  return 0;
}

void on_new_frame(toonz_node_handle_t node, const toonz_rendering_setting_t* rs,
                  double frame) {
  tnzu::Fx* fx = nullptr;
  nodeif->get_user_data(node, reinterpret_cast<void**>(&fx));

  DEBUG_PRINT(__FUNCTION__ << " : node=" << node << ", data=" << fx);
  if (!fx) {
    return;
  }

  fx->begin_frame();
}

void on_end_frame(toonz_node_handle_t node, const toonz_rendering_setting_t* rs,
                  double frame) {
  tnzu::Fx* fx = nullptr;
  nodeif->get_user_data(node, reinterpret_cast<void**>(&fx));

  DEBUG_PRINT(__FUNCTION__ << " : node=" << node << ", data=" << fx);
  if (!fx) {
    return;
  }

  fx->end_frame();
}

int node_create(toonz_node_handle_t node) {
  DEBUG_PRINT(__FUNCTION__ << " : " << node);

  tnzu::Fx* fx = nullptr;
  nodeif->get_user_data(node, reinterpret_cast<void**>(&fx));
  if (fx) {
    DEBUG_PRINT("this is clone");
  } else {
    fx = tnzu::make_fx();
    fx->handle() = node;
    nodeif->set_user_data(node, fx);
  }

  return fx->init();
}

int node_destroy(toonz_node_handle_t node) {
  tnzu::Fx* fx = nullptr;
  nodeif->get_user_data(node, reinterpret_cast<void**>(&fx));

  DEBUG_PRINT(__FUNCTION__ << " : node=" << node << ", data=" << fx);

  if (fx && (fx->handle() == node)) {
    nodeif->set_user_data(node, nullptr);
    delete fx;
    DEBUG_PRINT("release a user_data");
  }

  return TOONZ_OK;
}

int plugin_setup(toonz_node_handle_t dummy) {
  auto setup = grab_interf<toonz::setup_interface_t>(TOONZ_UUID_SETUP);
  if (!setup) {
    return TOONZ_ERROR_NOT_FOUND;
  }

  std::unique_ptr<tnzu::Fx> fx(tnzu::make_fx());

  // construct parameters
  static std::vector<std::vector<toonz_param_desc_t>> const params =
      build_params(fx);
  static std::vector<toonz_param_group_t> const groups =
      build_groups(fx, params);
  static toonz_param_page_t const pages =
      build_page("Properties", groups.size(), groups.data());

  // setup parameters
  int errcode = 0;
  void* entry = NULL;
  int ret = setup->set_parameter_pages_with_error(
      dummy, 1, const_cast<toonz_param_page_t*>(&pages), &errcode, &entry);
  if (ret) {
    DEBUG_PRINT("setup error:" << ret << " reason:" << errcode
                               << " entry:" << entry);
  }

  // add input ports
  for (int i = 0, argc = fx->port_count(); i < argc; i++) {
    setup->add_input_port(dummy, fx->port_name(i), TOONZ_PORT_TYPE_RASTER);
  }

  return TOONZ_OK;
}

int start_render(toonz_node_handle_t node) {
  tnzu::Fx* fx = nullptr;
  nodeif->get_user_data(node, reinterpret_cast<void**>(&fx));

  DEBUG_PRINT(__FUNCTION__ << " : node=" << node << ", data=" << fx);
  if (!fx) {
    return 1;
  }

  return fx->begin_render();
}

int end_render(toonz_node_handle_t node) {
  tnzu::Fx* fx = nullptr;
  nodeif->get_user_data(node, reinterpret_cast<void**>(&fx));

  DEBUG_PRINT(__FUNCTION__ << " : node=" << node << ", data=" << fx);
  if (!fx) {
    return 1;
  }

  return fx->end_render();
}

int toonz_plugin_init_main(toonz_host_interface_t* hostif) {
  DEBUG_PRINT(__FUNCTION__);
  ifactory = hostif;
  TOONZ_SET_IF(nodeif, toonz_node_interface_t, TOONZ_UUID_NODE);
  TOONZ_SET_IF(portif, toonz_port_interface_t, TOONZ_UUID_PORT);
  TOONZ_SET_IF(fxif, toonz_fxnode_interface_t, TOONZ_UUID_FXNODE);
  TOONZ_SET_IF(paramif, toonz_param_interface_t, TOONZ_UUID_PARAM);
  TOONZ_SET_IF(tileif, toonz_tile_interface_t, TOONZ_UUID_TILE);
  return TOONZ_OK;
}

void toonz_plugin_exit_main() { DEBUG_PRINT(__FUNCTION__); }
}
