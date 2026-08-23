#include "latituderuler_pi.h"
#include "version.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/gl.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include <wx/dcmemory.h>
#include <wx/fileconf.h>

#ifdef _WIN32
#define LATITUDERULER_EXPORT __declspec(dllexport)
#else
#define LATITUDERULER_EXPORT
#endif

namespace {

constexpr int kRulerWidth = 82;
constexpr int kMajorTickLength = 16;
constexpr int kMinorTickLength = 7;
constexpr int kLabelInset = 5;
constexpr double kPi = 3.14159265358979323846;

struct Theme {
  wxColour background;
  wxColour border;
  wxColour major;
  wxColour minor;
  wxColour text;
  wxColour marker;
  float glBackground[4];
  float glBorder[4];
  float glMajor[4];
  float glMinor[4];
  float glText[4];
  float glMarker[4];
};

Theme ThemeFor(PI_ColorScheme scheme) {
  Theme t;
  if (scheme == PI_GLOBAL_COLOR_SCHEME_NIGHT) {
    t.background = wxColour(35, 8, 8);
    t.border = wxColour(130, 45, 40);
    t.major = wxColour(220, 95, 75);
    t.minor = wxColour(130, 55, 48);
    t.text = wxColour(255, 170, 135);
    t.marker = wxColour(255, 205, 80);
    const float bg[] = {0.08f, 0.01f, 0.01f, 0.88f};
    const float border[] = {0.50f, 0.12f, 0.10f, 0.95f};
    const float major[] = {0.92f, 0.32f, 0.24f, 0.98f};
    const float minor[] = {0.50f, 0.17f, 0.14f, 0.85f};
    const float text[] = {1.00f, 0.62f, 0.48f, 1.0f};
    const float marker[] = {1.00f, 0.78f, 0.20f, 1.0f};
    std::copy(bg, bg + 4, t.glBackground);
    std::copy(border, border + 4, t.glBorder);
    std::copy(major, major + 4, t.glMajor);
    std::copy(minor, minor + 4, t.glMinor);
    std::copy(text, text + 4, t.glText);
    std::copy(marker, marker + 4, t.glMarker);
  } else if (scheme == PI_GLOBAL_COLOR_SCHEME_DUSK) {
    t.background = wxColour(75, 64, 40);
    t.border = wxColour(145, 125, 78);
    t.major = wxColour(225, 205, 145);
    t.minor = wxColour(145, 126, 82);
    t.text = wxColour(255, 235, 175);
    t.marker = wxColour(95, 185, 255);
    const float bg[] = {0.20f, 0.17f, 0.09f, 0.86f};
    const float border[] = {0.53f, 0.44f, 0.24f, 0.95f};
    const float major[] = {0.90f, 0.81f, 0.54f, 0.98f};
    const float minor[] = {0.56f, 0.47f, 0.27f, 0.82f};
    const float text[] = {1.00f, 0.92f, 0.68f, 1.0f};
    const float marker[] = {0.30f, 0.72f, 1.00f, 1.0f};
    std::copy(bg, bg + 4, t.glBackground);
    std::copy(border, border + 4, t.glBorder);
    std::copy(major, major + 4, t.glMajor);
    std::copy(minor, minor + 4, t.glMinor);
    std::copy(text, text + 4, t.glText);
    std::copy(marker, marker + 4, t.glMarker);
  } else {
    t.background = wxColour(244, 244, 238);
    t.border = wxColour(105, 110, 112);
    t.major = wxColour(40, 48, 52);
    t.minor = wxColour(125, 130, 132);
    t.text = wxColour(15, 20, 22);
    t.marker = wxColour(25, 115, 210);
    const float bg[] = {0.96f, 0.96f, 0.93f, 0.88f};
    const float border[] = {0.36f, 0.39f, 0.40f, 0.96f};
    const float major[] = {0.10f, 0.13f, 0.15f, 0.98f};
    const float minor[] = {0.43f, 0.46f, 0.47f, 0.82f};
    const float text[] = {0.03f, 0.04f, 0.05f, 1.0f};
    const float marker[] = {0.08f, 0.38f, 0.88f, 1.0f};
    std::copy(bg, bg + 4, t.glBackground);
    std::copy(border, border + 4, t.glBorder);
    std::copy(major, major + 4, t.glMajor);
    std::copy(minor, minor + 4, t.glMinor);
    std::copy(text, text + 4, t.glText);
    std::copy(marker, marker + 4, t.glMarker);
  }
  return t;
}

bool LatitudeAtY(PlugIn_ViewPort *vp, int y, double *lat) {
  if (!vp || !lat || vp->pix_height <= 1) return false;
  double lon = 0.0;
  GetCanvasLLPix(vp, wxPoint(kRulerWidth, y), lat, &lon);
  return std::isfinite(*lat);
}

bool VisibleLatitudeRange(PlugIn_ViewPort *vp, double *latTop,
                          double *latBottom) {
  if (!vp || !latTop || !latBottom || vp->pix_height < 2) return false;
  if (!LatitudeAtY(vp, 0, latTop)) return false;
  if (!LatitudeAtY(vp, vp->pix_height - 1, latBottom)) return false;
  return std::isfinite(*latTop) && std::isfinite(*latBottom) &&
         std::fabs(*latTop - *latBottom) > 1.0e-10;
}

int FindYForLatitude(PlugIn_ViewPort *vp, double target, double latTop,
                     double latBottom) {
  if (!vp || vp->pix_height < 2) return -1;
  const bool decreasing = latTop > latBottom;
  int low = 0;
  int high = vp->pix_height - 1;

  for (int i = 0; i < 16 && high - low > 1; ++i) {
    const int mid = low + (high - low) / 2;
    double latMid = 0.0;
    if (!LatitudeAtY(vp, mid, &latMid)) return -1;
    if ((decreasing && latMid > target) || (!decreasing && latMid < target))
      low = mid;
    else
      high = mid;
  }

  double latLow = 0.0;
  double latHigh = 0.0;
  if (!LatitudeAtY(vp, low, &latLow) || !LatitudeAtY(vp, high, &latHigh))
    return -1;
  return std::fabs(latLow - target) <= std::fabs(latHigh - target) ? low : high;
}

double ChooseMajorStep(double spanDegrees, int heightPixels) {
  if (!(spanDegrees > 0.0) || heightPixels <= 0) return 1.0;
  const double desired = spanDegrees * 82.0 / static_cast<double>(heightPixels);
  static const double steps[] = {
      1.0 / 3600.0, 2.0 / 3600.0, 5.0 / 3600.0,
      10.0 / 3600.0, 15.0 / 3600.0, 30.0 / 3600.0,
      1.0 / 60.0, 2.0 / 60.0, 5.0 / 60.0, 10.0 / 60.0,
      15.0 / 60.0, 30.0 / 60.0, 1.0, 2.0, 5.0, 10.0, 15.0,
      30.0, 45.0, 90.0};
  for (double step : steps) {
    if (step >= desired) return step;
  }
  return 90.0;
}

wxString FormatLatitude(double lat, double majorStep) {
  const bool north = lat >= 0.0;
  double value = std::fabs(lat);
  int deg = static_cast<int>(std::floor(value + 1.0e-10));
  double minFull = (value - deg) * 60.0;
  int min = static_cast<int>(std::floor(minFull + 1.0e-8));
  int sec = static_cast<int>(std::lround((minFull - min) * 60.0));
  if (sec >= 60) {
    sec = 0;
    ++min;
  }
  if (min >= 60) {
    min = 0;
    ++deg;
  }

  const wxString degree = wxString::FromUTF8("°");
  const wxString hemi = north ? wxT("N") : wxT("S");
  if (majorStep >= 1.0 - 1.0e-10)
    return wxString::Format(wxT("%d"), deg) + degree + wxT(" ") + hemi;
  if (majorStep >= 1.0 / 60.0 - 1.0e-10)
    return wxString::Format(wxT("%d"), deg) + degree +
           wxString::Format(wxT("%02d' "), min) + hemi;
  return wxString::Format(wxT("%d"), deg) + degree +
         wxString::Format(wxT("%02d'%02d\" "), min, sec) + hemi;
}

std::string FormatLatitudeAscii(double lat, double majorStep) {
  const bool north = lat >= 0.0;
  double value = std::fabs(lat);
  int deg = static_cast<int>(std::floor(value + 1.0e-10));
  double minFull = (value - deg) * 60.0;
  int min = static_cast<int>(std::floor(minFull + 1.0e-8));
  int sec = static_cast<int>(std::lround((minFull - min) * 60.0));
  if (sec >= 60) {
    sec = 0;
    ++min;
  }
  if (min >= 60) {
    min = 0;
    ++deg;
  }

  char buffer[32];
  if (majorStep >= 1.0 - 1.0e-10)
    std::snprintf(buffer, sizeof(buffer), "%do%c", deg, north ? 'N' : 'S');
  else if (majorStep >= 1.0 / 60.0 - 1.0e-10)
    std::snprintf(buffer, sizeof(buffer), "%do%02d'%c", deg, min,
                  north ? 'N' : 'S');
  else
    std::snprintf(buffer, sizeof(buffer), "%do%02d'%02d\"%c", deg, min, sec,
                  north ? 'N' : 'S');
  return std::string(buffer);
}

struct Tick {
  double latitude = 0.0;
  int y = 0;
  bool major = false;
};

std::vector<Tick> BuildTicks(PlugIn_ViewPort *vp, double *majorStepOut) {
  std::vector<Tick> ticks;
  double latTop = 0.0;
  double latBottom = 0.0;
  if (!VisibleLatitudeRange(vp, &latTop, &latBottom)) return ticks;

  const double minLat = std::max(-90.0, std::min(latTop, latBottom));
  const double maxLat = std::min(90.0, std::max(latTop, latBottom));
  const double span = maxLat - minLat;
  const double majorStep = ChooseMajorStep(span, vp->pix_height);
  const double minorStep = majorStep / 5.0;
  if (majorStepOut) *majorStepOut = majorStep;

  const long long firstIndex =
      static_cast<long long>(std::ceil((minLat - 1.0e-12) / minorStep));
  const long long lastIndex =
      static_cast<long long>(std::floor((maxLat + 1.0e-12) / minorStep));

  if (lastIndex - firstIndex > 1000) return ticks;

  for (long long i = firstIndex; i <= lastIndex; ++i) {
    const double lat = i * minorStep;
    const int y = FindYForLatitude(vp, lat, latTop, latBottom);
    if (y < -2 || y > vp->pix_height + 2) continue;
    const long long majorIndex =
        static_cast<long long>(std::llround(lat / majorStep));
    const bool major =
        std::fabs(lat - majorIndex * majorStep) < minorStep * 0.08;
    ticks.push_back({lat, y, major});
  }
  return ticks;
}

void SetGLColour(const float c[4]) { glColor4f(c[0], c[1], c[2], c[3]); }

void DrawSegment(float x1, float y1, float x2, float y2) {
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
}

unsigned char DigitSegments(char c) {
  static const unsigned char map[10] = {
      0x3F, 0x06, 0x5B, 0x4F, 0x66,
      0x6D, 0x7D, 0x07, 0x7F, 0x6F};
  return (c >= '0' && c <= '9') ? map[c - '0'] : 0;
}

void DrawVectorGlyphGL(char c, float x, float y, float s) {
  const float w = 4.0f * s;
  const float h = 7.0f * s;
  if (c >= '0' && c <= '9') {
    const unsigned char m = DigitSegments(c);
    glBegin(GL_LINES);
    if (m & 0x01) DrawSegment(x, y, x + w, y);
    if (m & 0x02) DrawSegment(x + w, y, x + w, y + h / 2);
    if (m & 0x04) DrawSegment(x + w, y + h / 2, x + w, y + h);
    if (m & 0x08) DrawSegment(x, y + h, x + w, y + h);
    if (m & 0x10) DrawSegment(x, y + h / 2, x, y + h);
    if (m & 0x20) DrawSegment(x, y, x, y + h / 2);
    if (m & 0x40) DrawSegment(x, y + h / 2, x + w, y + h / 2);
    glEnd();
    return;
  }

  glBegin(GL_LINES);
  if (c == 'N') {
    DrawSegment(x, y + h, x, y);
    DrawSegment(x, y, x + w, y + h);
    DrawSegment(x + w, y + h, x + w, y);
  } else if (c == 'S') {
    DrawSegment(x + w, y, x, y);
    DrawSegment(x, y, x, y + h / 2);
    DrawSegment(x, y + h / 2, x + w, y + h / 2);
    DrawSegment(x + w, y + h / 2, x + w, y + h);
    DrawSegment(x + w, y + h, x, y + h);
  } else if (c == '\'') {
    DrawSegment(x + w * 0.55f, y, x + w * 0.40f, y + h * 0.26f);
  } else if (c == '"') {
    DrawSegment(x + w * 0.30f, y, x + w * 0.22f, y + h * 0.24f);
    DrawSegment(x + w * 0.75f, y, x + w * 0.67f, y + h * 0.24f);
  }
  glEnd();

  if (c == 'o') {
    const float cx = x + w * 0.52f;
    const float cy = y + h * 0.20f;
    const float r = 1.15f * s;
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 12; ++i) {
      const float a = static_cast<float>(i) * 2.0f *
                      static_cast<float>(kPi) / 12.0f;
      glVertex2f(cx + r * std::cos(a), cy + r * std::sin(a));
    }
    glEnd();
  }
}

void DrawVectorTextGL(const std::string &text, float x, float y, float s,
                      const float colour[4]) {
  SetGLColour(colour);
  glLineWidth(std::max(1.4f, 1.25f * s));
  float cursor = x;
  for (char c : text) {
    if (c == ' ') {
      cursor += 3.0f * s;
      continue;
    }
    DrawVectorGlyphGL(c, cursor, y, s);
    cursor += 5.5f * s;
  }
}

}  // namespace

extern "C" LATITUDERULER_EXPORT opencpn_plugin *create_pi(void *ppimgr) {
  return new LatitudeRulerPi(ppimgr);
}

extern "C" LATITUDERULER_EXPORT void destroy_pi(opencpn_plugin *p) {
  delete p;
}

LatitudeRulerPi::LatitudeRulerPi(void *ppimgr) : opencpn_plugin_118(ppimgr) {}

int LatitudeRulerPi::Init() {
  LoadConfig();
  BuildToolbarBitmap();
  m_toolbarId = InsertPlugInTool(wxT(""), &m_toolbarBitmap, &m_toolbarBitmap,
                                 wxITEM_CHECK, wxT("Latitude Ruler"),
                                 wxT("Show latitude ruler"), nullptr, -1, 0,
                                 this);
  if (m_toolbarId >= 0) SetToolbarItemState(m_toolbarId, m_enabled);

  return INSTALLS_TOOLBAR_TOOL | WANTS_TOOLBAR_CALLBACK |
         WANTS_OVERLAY_CALLBACK | WANTS_OPENGL_OVERLAY_CALLBACK |
         WANTS_MOUSE_EVENTS;
}

bool LatitudeRulerPi::DeInit() {
  SaveConfig();
  if (m_toolbarId >= 0) {
    RemovePlugInTool(m_toolbarId);
    m_toolbarId = -1;
  }
  return true;
}

int LatitudeRulerPi::GetAPIVersionMajor() { return OCPN_API_VERSION_MAJOR; }
int LatitudeRulerPi::GetAPIVersionMinor() { return OCPN_API_VERSION_MINOR; }
int LatitudeRulerPi::GetPlugInVersionMajor() { return PLUGIN_VERSION_MAJOR; }
int LatitudeRulerPi::GetPlugInVersionMinor() { return PLUGIN_VERSION_MINOR; }
int LatitudeRulerPi::GetPlugInVersionPatch() { return PLUGIN_VERSION_PATCH; }
int LatitudeRulerPi::GetPlugInVersionPost() { return PLUGIN_VERSION_POST; }
int LatitudeRulerPi::GetToolbarToolCount() { return 1; }

wxBitmap *LatitudeRulerPi::GetPlugInBitmap() { return &m_pluginBitmap; }
wxString LatitudeRulerPi::GetCommonName() { return wxT("Latitude Ruler"); }
wxString LatitudeRulerPi::GetShortDescription() {
  return wxT("Adaptive latitude ruler on the left side of the chart");
}
wxString LatitudeRulerPi::GetLongDescription() {
  return wxT("Shows a nautical latitude scale along the left chart edge. "
             "Tick spacing follows the current OpenCPN viewport and zoom level.");
}

void LatitudeRulerPi::BuildToolbarBitmap() {
  m_toolbarBitmap = wxBitmap(32, 32, 32);
  wxMemoryDC dc(m_toolbarBitmap);
  dc.SetBackground(wxBrush(wxColour(45, 54, 58)));
  dc.Clear();
  dc.SetPen(wxPen(wxColour(235, 240, 242), 2));
  dc.DrawLine(9, 4, 9, 28);
  for (int y = 6; y <= 26; y += 4) {
    const int len = (y % 8 == 6) ? 11 : 7;
    dc.DrawLine(9, y, 9 + len, y);
  }
  dc.SetTextForeground(wxColour(235, 240, 242));
  wxFont font(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
  dc.SetFont(font);
  dc.DrawText(wxT("N"), 21, 10);
  dc.SelectObject(wxNullBitmap);
  m_pluginBitmap = m_toolbarBitmap;
}

void LatitudeRulerPi::LoadConfig() {
  wxFileConfig *config = GetOCPNConfigObject();
  if (!config) return;
  config->SetPath(wxT("/PlugIns/LatitudeRuler"));
  config->Read(wxT("Enabled"), &m_enabled, true);
}

void LatitudeRulerPi::SaveConfig() {
  wxFileConfig *config = GetOCPNConfigObject();
  if (!config) return;
  config->SetPath(wxT("/PlugIns/LatitudeRuler"));
  config->Write(wxT("Enabled"), m_enabled);
  config->Flush();
}

void LatitudeRulerPi::OnToolbarToolCallback(int id) {
  if (id != m_toolbarId) return;
  m_enabled = !m_enabled;
  if (m_toolbarId >= 0) SetToolbarItemState(m_toolbarId, m_enabled);
  SaveConfig();
  wxWindow *canvas = GetOCPNCanvasWindow();
  if (canvas) RequestRefresh(canvas);
}

bool LatitudeRulerPi::MouseEventHook(wxMouseEvent &event) {
  const bool inside = m_enabled && !event.Leaving() && event.GetY() >= 0;
  const int newY = inside ? event.GetY() : -1;
  const bool changed = (inside != m_mouseInside) || (newY != m_mouseY);

  m_mouseInside = inside;
  m_mouseY = newY;

  if (changed) {
    wxWindow *canvas = GetOCPNCanvasWindow();
    if (canvas) RequestRefresh(canvas);
  }

  return false;
}

void LatitudeRulerPi::SetColorScheme(PI_ColorScheme cs) {
  m_colorScheme = cs;
  wxWindow *canvas = GetOCPNCanvasWindow();
  if (canvas) RequestRefresh(canvas);
}

bool LatitudeRulerPi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp) {
  if (!m_enabled || !vp || !vp->bValid || vp->pix_height < 10) return false;

  double majorStep = 1.0;
  const std::vector<Tick> ticks = BuildTicks(vp, &majorStep);
  if (ticks.empty()) return false;

  const Theme theme = ThemeFor(m_colorScheme);
  dc.SetPen(*wxTRANSPARENT_PEN);
  dc.SetBrush(wxBrush(theme.background));
  dc.DrawRectangle(0, 0, kRulerWidth, vp->pix_height);

  dc.SetPen(wxPen(theme.border, 1));
  dc.DrawLine(kRulerWidth - 1, 0, kRulerWidth - 1, vp->pix_height);

  wxFont font = *wxNORMAL_FONT;
  font.SetPointSize(std::max(9, font.GetPointSize()));
  font.SetWeight(wxFONTWEIGHT_BOLD);
  dc.SetFont(font);
  dc.SetTextForeground(theme.text);
  dc.SetBackgroundMode(wxTRANSPARENT);

  for (const Tick &tick : ticks) {
    const int x2 = kRulerWidth - 1;
    const int length = tick.major ? kMajorTickLength : kMinorTickLength;
    dc.SetPen(wxPen(tick.major ? theme.major : theme.minor,
                    tick.major ? 2 : 1));
    dc.DrawLine(x2 - length, tick.y, x2, tick.y);

    if (tick.major) {
      const wxString label = FormatLatitude(tick.latitude, majorStep);
      wxCoord tw = 0, th = 0;
      dc.GetTextExtent(label, &tw, &th);
      const int labelY = tick.y - static_cast<int>(th) / 2;
      if (labelY >= -2 && labelY + th <= vp->pix_height + 2)
        dc.DrawText(label, kLabelInset, labelY);
    }
  }

  if (m_mouseInside && m_mouseY >= 0 && m_mouseY < vp->pix_height) {
    const int y = m_mouseY;
    wxPoint marker[3] = {wxPoint(kRulerWidth - 1, y),
                         wxPoint(kRulerWidth - 13, y - 6),
                         wxPoint(kRulerWidth - 13, y + 6)};
    dc.SetPen(wxPen(theme.marker, 1));
    dc.SetBrush(wxBrush(theme.marker));
    dc.DrawPolygon(3, marker);
  }

  return true;
}

bool LatitudeRulerPi::RenderGLOverlayMultiCanvas(wxGLContext *pcontext,
                                                  PlugIn_ViewPort *vp,
                                                  int canvasIndex,
                                                  int priority) {
  (void)pcontext;
  (void)canvasIndex;
  if (priority != -1 && priority != OVERLAY_LEGACY) return false;
  if (!m_enabled || !vp || !vp->bValid || vp->pix_height < 10) return false;

  double majorStep = 1.0;
  const std::vector<Tick> ticks = BuildTicks(vp, &majorStep);
  if (ticks.empty()) return false;
  const Theme theme = ThemeFor(m_colorScheme);

  glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT |
               GL_TRANSFORM_BIT | GL_VIEWPORT_BIT | GL_CURRENT_BIT);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0, static_cast<double>(vp->pix_width),
          static_cast<double>(vp->pix_height), 0.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  SetGLColour(theme.glBackground);
  glBegin(GL_QUADS);
  glVertex2f(0.0f, 0.0f);
  glVertex2f(static_cast<float>(kRulerWidth), 0.0f);
  glVertex2f(static_cast<float>(kRulerWidth),
             static_cast<float>(vp->pix_height));
  glVertex2f(0.0f, static_cast<float>(vp->pix_height));
  glEnd();

  SetGLColour(theme.glBorder);
  glLineWidth(1.0f);
  glBegin(GL_LINES);
  DrawSegment(static_cast<float>(kRulerWidth - 1), 0.0f,
              static_cast<float>(kRulerWidth - 1),
              static_cast<float>(vp->pix_height));
  glEnd();

  for (const Tick &tick : ticks) {
    SetGLColour(tick.major ? theme.glMajor : theme.glMinor);
    glLineWidth(tick.major ? 2.0f : 1.0f);
    const float x2 = static_cast<float>(kRulerWidth - 1);
    const float length =
        static_cast<float>(tick.major ? kMajorTickLength : kMinorTickLength);
    glBegin(GL_LINES);
    DrawSegment(x2 - length, static_cast<float>(tick.y), x2,
                static_cast<float>(tick.y));
    glEnd();

    if (tick.major) {
      const std::string label = FormatLatitudeAscii(tick.latitude, majorStep);
      const float scale = 1.38f;
      const float labelHeight = 7.0f * scale;
      const float y = static_cast<float>(tick.y) - labelHeight * 0.5f;
      if (y >= -2.0f && y + labelHeight <= vp->pix_height + 2.0f)
        DrawVectorTextGL(label, static_cast<float>(kLabelInset), y, scale,
                         theme.glText);
    }
  }

  if (m_mouseInside && m_mouseY >= 0 && m_mouseY < vp->pix_height) {
    const float y = static_cast<float>(m_mouseY);
    SetGLColour(theme.glMarker);
    glBegin(GL_TRIANGLES);
    glVertex2f(static_cast<float>(kRulerWidth - 1), y);
    glVertex2f(static_cast<float>(kRulerWidth - 13), y - 6.0f);
    glVertex2f(static_cast<float>(kRulerWidth - 13), y + 6.0f);
    glEnd();
  }

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopAttrib();
  return true;
}
