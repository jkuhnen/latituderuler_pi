#ifndef LATITUDERULER_PI_H
#define LATITUDERULER_PI_H

#include <wx/wx.h>
#include "ocpn_plugin.h"

class LatitudeRulerPi : public opencpn_plugin_118 {
public:
  explicit LatitudeRulerPi(void *ppimgr);
  ~LatitudeRulerPi() override = default;

  int Init() override;
  bool DeInit() override;

  int GetAPIVersionMajor() override;
  int GetAPIVersionMinor() override;
  int GetPlugInVersionMajor() override;
  int GetPlugInVersionMinor() override;
  int GetPlugInVersionPatch() override;
  int GetPlugInVersionPost() override;
  int GetToolbarToolCount() override;

  wxBitmap *GetPlugInBitmap() override;
  wxString GetCommonName() override;
  wxString GetShortDescription() override;
  wxString GetLongDescription() override;

  void OnToolbarToolCallback(int id) override;
  void SetColorScheme(PI_ColorScheme cs) override;
  bool MouseEventHook(wxMouseEvent &event) override;

  bool RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp) override;
  bool RenderGLOverlayMultiCanvas(wxGLContext *pcontext, PlugIn_ViewPort *vp,
                                  int canvasIndex, int priority = -1) override;

private:
  void BuildToolbarBitmap();
  void LoadConfig();
  void SaveConfig();

  wxBitmap m_pluginBitmap;
  wxBitmap m_toolbarBitmap;
  int m_toolbarId = -1;
  bool m_enabled = true;
  PI_ColorScheme m_colorScheme = PI_GLOBAL_COLOR_SCHEME_DAY;
  int m_mouseY = -1;
  bool m_mouseInside = false;
};

#endif
