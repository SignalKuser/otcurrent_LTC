/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  otcurrent Object
 * Author:   David Register, Mike Rossiter
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 ***************************************************************************
 *
 */
#include <wx/intl.h>
#include "wx/wx.h"
#include "wx/datetime.h"
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/debug.h>
#include <wx/graphics.h>
#include <wx/stdpaths.h>

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "otcurrent_pi.h"
#include "folder.xpm"
#include "icons.h"
#include <wx/arrimpl.cpp>

#ifdef __WXMSW__
#include <windows.h>
#endif
#include <memory.h>

#include <wx/colordlg.h>
#include <wx/choicdlg.h>
#include <wx/tokenzr.h>
#include "otcurrent_pi.h"
#include "qtstylesheet.h"

#ifdef __ANDROID__
wxWindow* g_Window;
#endif

using namespace std;

#define FAIL(X)  \
  do {           \
    error = X;   \
    goto failed; \
  } while (0)

// date/time in the desired time zone format
static wxString TToString(const wxDateTime date_time, const int time_zone) {
  wxDateTime t(date_time);
  t.MakeFromTimezone(wxDateTime::UTC);
  if (t.IsDST()) t.Subtract(wxTimeSpan(1, 0, 0, 0));
  switch (time_zone) {
    case 0:
      return t.Format(_T(" %a %d-%b-%Y  %H:%M LOC"), wxDateTime::Local);
    case 1:
    default:
      return t.Format(_T(" %a %d-%b-%Y %H:%M  UTC"), wxDateTime::UTC);
  }
}

static wxArrayString FindTcdFiles(const wxString& folder) {
  wxArrayString files;
  wxDir data_dir(folder);
  if (!data_dir.IsOpened()) return files;

  wxString file_name;
  bool found = data_dir.GetFirst(&file_name, wxEmptyString, wxDIR_FILES);
  while (found) {
    wxFileName file_path(folder, file_name);
    if (file_path.GetExt().CmpNoCase(_T("tcd")) == 0)
      files.Add(file_name);
    found = data_dir.GetNext(&file_name);
  }

  files.Sort();
  return files;
}

static bool ArrayContainsNoCase(const wxArrayString& values,
                                const wxString& value) {
  for (unsigned int i = 0; i < values.GetCount(); i++) {
    if (values[i].CmpNoCase(value) == 0) return true;
  }
  return false;
}

static wxArrayString FindOpenCpnIdxSources() {
  wxArrayString files;
  wxFileConfig* config = GetOCPNConfigObject();
  if (!config) return files;

  const wxString previous_path = config->GetPath();
  config->SetPath(_T("/TideCurrentDataSources"));

  wxString entry_name;
  long cookie = 0;
  bool found = config->GetFirstEntry(entry_name, cookie);
  while (found) {
    wxString source_path;
    if (config->Read(entry_name, &source_path)) {
      wxFileName source(source_path);
      if (source.GetExt().CmpNoCase(_T("idx")) == 0 &&
          source.FileExists() && !ArrayContainsNoCase(files, source_path)) {
        files.Add(source_path);
      }
    }
    found = config->GetNextEntry(entry_name, cookie);
  }

  config->SetPath(previous_path);
  files.Sort();
  return files;
}

static bool SelectionContains(const wxString& setting,
                              const wxString& file_name) {
  if (setting == _T("*")) return true;
  if (setting == _T("!")) return false;

  wxStringTokenizer tokenizer(setting, _T(";"));
  while (tokenizer.HasMoreTokens()) {
    if (tokenizer.GetNextToken().CmpNoCase(file_name) == 0) return true;
  }
  return false;
}

#if !wxCHECK_VERSION(2, 9, 4) /* to work with wx 2.8 */
#define SetBitmap SetBitmapLabel
#endif

otcurrentUIDialog::otcurrentUIDialog(wxWindow* parent, otcurrent_pi* ppi)
    : otcurrentUIDialogBase(parent), ptcmgr(0), m_SourceMode(0), m_vp(0) {
  this->Fit();
  pParent = parent;
  pPlugIn = ppi;

  wxFileConfig* pConf = GetOCPNConfigObject();
#ifdef __ANDROID__

  g_Window = this;
  GetHandle()->setStyleSheet(qtStyleSheet);
  
  Connect(wxEVT_MOTION, wxMouseEventHandler(otcurrentUIDialog::OnMouseEvent));

#endif

  if (pConf) {
    pConf->SetPath(_T ( "/PlugIns/otcurrent_ltc_pi" ));

    pConf->Read(_T ( "otcurrentUseRate" ), &m_bUseRate);
    pConf->Read(_T ( "otcurrentUseDirection" ), &m_bUseDirection);
    pConf->Read(_T("otcurrentUseHighResolution"), &m_bUseHighRes);
    pConf->Read(_T ( "otcurrentUseFillColour" ), &m_bUseFillColour);
    pConf->Read("otcurrentArrowScale", &m_arrow_scale);
    pConf->Read(_T("otcurrentTcdFiles"), &m_TcdSelectionSetting, _T("*"));
    pConf->Read(_T("otcurrentSourceMode"), &m_SourceMode, 0);

    pConf->Read(_T("VColour0"), &myVColour[0], myVColour[0]);
    pConf->Read(_T("VColour1"), &myVColour[1], myVColour[1]);
    pConf->Read(_T("VColour2"), &myVColour[2], myVColour[2]);
    pConf->Read(_T("VColour3"), &myVColour[3], myVColour[3]);
    pConf->Read(_T("VColour4"), &myVColour[4], myVColour[4]);

    myUseColour[0] = myVColour[0];
    myUseColour[1] = myVColour[1];
    myUseColour[2] = myVColour[2];
    myUseColour[3] = myVColour[3];
    myUseColour[4] = myVColour[4];
  }

  m_bpPrev->SetBitmap(wxBitmap(prev1));
  m_bpNext->SetBitmap(wxBitmap(next1));
  m_bpNow->SetBitmap(*_img_Clock);

  // this->Connect( wxEVT_MOVE, wxMoveEventHandler( otcurrentUIDialog::OnMove )
  // );
  m_dtNow = wxDateTime::Now();
  MakeDateTimeLabel(m_dtNow);

  m_IntervalSelected = pPlugIn->GetIntervalSelected();
  m_FolderSelected = pPlugIn->GetFolderSelected();

  m_dirPicker1->SetValue(m_FolderSelected);
  if (m_SourceMode < 0 || m_SourceMode > 2) m_SourceMode = 0;
  m_sourceModeChoice->SetSelection(m_SourceMode);
  const bool tcd_mode = m_SourceMode != 0;
  m_dirPicker1->Enable(tcd_mode);
  m_button2->Enable(tcd_mode);
  m_choice1->SetSelection(m_IntervalSelected);

  LoadTCMFile();
  LoadHarmonics();

  int i = m_choice1->GetSelection();
  wxString c = m_choice1->GetString(i);
  double value;
  c.ToDouble(&value);
  m_dInterval = value;


}

#ifdef __ANDROID__
wxPoint g_mouse_pos_screen;

void otcurrentUIDialog::OnMouseEvent(wxMouseEvent& event) {
  g_mouse_pos_screen = ClientToScreen(event.GetPosition());

  if (event.Dragging()) {
    m_resizeStartPoint = event.GetPosition();
    int x = wxMax(0, m_resizeStartPoint.x);
    int y = wxMax(0, m_resizeStartPoint.y);
    int xmax = ::wxGetDisplaySize().x - GetSize().x;
    x = wxMin(x, xmax);
    int ymax =
        ::wxGetDisplaySize().y - (GetSize().y);  // Some fluff at the bottom
    y = wxMin(y, ymax);

    g_Window->Move(x, y);
  }
}
#endif  // End of Android functions for move/resize



void otcurrentUIDialog::LoadHarmonics() {
  if (!ptcmgr) {
    ptcmgr = new TCMgr;
    ptcmgr->LoadDataSources(TideCurrentDataSet);
  } else {
    bool b_newdataset = false;

    //      Test both ways
    wxArrayString test = ptcmgr->GetDataSet();
    for (unsigned int i = 0; i < test.GetCount(); i++) {
      bool b_foundi = false;
      for (unsigned int j = 0; j < TideCurrentDataSet.GetCount(); j++) {
        if (TideCurrentDataSet[j] == test[i]) {
          b_foundi = true;
          break;  // j loop
        }
      }
      if (!b_foundi) {
        b_newdataset = true;
        break;  //  i loop
      }
    }

    test = TideCurrentDataSet;
    for (unsigned int i = 0; i < test.GetCount(); i++) {
      bool b_foundi = false;
      for (unsigned int j = 0; j < ptcmgr->GetDataSet().GetCount(); j++) {
        if (ptcmgr->GetDataSet()[j] == test[i]) {
          b_foundi = true;
          break;  // j loop
        }
      }
      if (!b_foundi) {
        b_newdataset = true;
        break;  //  i loop
      }
    }

    if (b_newdataset) ptcmgr->LoadDataSources(TideCurrentDataSet);
  }
}

void otcurrentUIDialog::LoadTCMFile() {
  TideCurrentDataSet.Clear();

  if (m_SourceMode == 0 || m_SourceMode == 2) {
    wxArrayString opencpn_idx_sources = FindOpenCpnIdxSources();
    for (unsigned int i = 0; i < opencpn_idx_sources.GetCount(); i++) {
      TideCurrentDataSet.Add(opencpn_idx_sources[i]);
      wxLogMessage(_("Using OpenCPN IDX Tide/Current data:  ") +
                   opencpn_idx_sources[i]);
    }
  }

  if (m_SourceMode == 1 || m_SourceMode == 2) {
    wxDir data_dir(m_FolderSelected);
    if (!data_dir.IsOpened()) {
      wxLogWarning(_("Cannot open Tide/Current data directory:  ") +
                   m_FolderSelected);
    } else {
      wxArrayString tcd_files = FindTcdFiles(m_FolderSelected);
      for (unsigned int i = 0; i < tcd_files.GetCount(); i++) {
        if (!SelectionContains(m_TcdSelectionSetting, tcd_files[i])) continue;
        wxString full_path =
            wxFileName(m_FolderSelected, tcd_files[i]).GetFullPath();
        TideCurrentDataSet.Add(full_path);
        wxLogMessage(_("Using TCD Tide/Current data:  ") + full_path);
      }
    }
  }

  if (TideCurrentDataSet.IsEmpty()) {
    wxString warning;
    if (m_SourceMode == 0)
      warning = _("No existing IDX sources are configured in OpenCPN.");
    else if (m_SourceMode == 1)
      warning = _("No TCD files are selected in the plugin.");
    else
      warning = _("No OpenCPN IDX or selected TCD sources are available.");
    wxLogWarning(warning);
  } else {
    wxLogMessage(wxString::Format(
        _("Loaded %u Tide/Current data source(s) from:  "),
        static_cast<unsigned int>(TideCurrentDataSet.GetCount())) +
                 m_FolderSelected);
  }
}

otcurrentUIDialog::~otcurrentUIDialog() {
  wxFileConfig* pConf = GetOCPNConfigObject();

  if (pConf) {
    pConf->SetPath(_T ( "/PlugIns/otcurrent_ltc_pi" ));

    pConf->Write(_T ( "otcurrentUseRate" ), m_bUseRate);
    pConf->Write(_T ( "otcurrentUseDirection" ), m_bUseDirection);
    pConf->Write(_T("otcurrentUseHighResolution"), m_bUseHighRes);
    pConf->Write(_T ( "otcurrentUseFillColour" ), m_bUseFillColour);
    pConf->Write("otcurrentArrowScale", m_arrow_scale);
    pConf->Write(_T("otcurrentTcdFiles"), m_TcdSelectionSetting);
    pConf->Write(_T("otcurrentSourceMode"), m_SourceMode);

    pConf->Write(_T("VColour0"), myVColour[0]);
    pConf->Write(_T("VColour1"), myVColour[1]);
    pConf->Write(_T("VColour2"), myVColour[2]);
    pConf->Write(_T("VColour3"), myVColour[3]);
    pConf->Write(_T("VColour4"), myVColour[4]);

    int c = m_choice1->GetSelection();
    m_IntervalSelected = c;
    wxString myP = m_choice1->GetString(c);
    pConf->Write(_T ( "otcurrentInterval" ), myP);

    wxString myF = m_dirPicker1->GetValue();
    pConf->Write(_T ( "otcurrentFolder" ), myF);
  }
  delete ptcmgr;
}

void otcurrentUIDialog::SetCursorLatLon(double lat, double lon) {
  m_cursor_lon = lon;
  m_cursor_lat = lat;
}

void otcurrentUIDialog::SetScaledBitmaps(double scalefactor) {
  //  Round to the nearest "quarter", to avoid rendering artifacts
  double myscaledFactor = wxRound(scalefactor * 4.0) / 4.0;
  int w, h;
  w = 32 * scalefactor;  // 32x32 is the standard bitmap's size
  h = 32 * scalefactor;

#ifdef ocpnUSE_SVG
  wxBitmap bitmap = GetBitmapFromSVGFile(_svg_otcurrent_prefs, w, h);
  m_button8->SetBitmap(bitmap);

#else
  wxImage im0 =
      wxBitmap(prev_blue).ConvertToImage().Scale(w, h, wxIMAGE_QUALITY_HIGH);
  m_bpPrev->SetBitmap(wxBitmap(im0));
  wxImage im1 =
      wxBitmap(next_blue).ConvertToImage().Scale(w, h, wxIMAGE_QUALITY_HIGH);
  m_bpNext->SetBitmap(wxBitmap(im1));
  wxImage im2 =
      wxBitmap(info_blue).ConvertToImage().Scale(w, h, wxIMAGE_QUALITY_HIGH);
  m_button8->SetBitmap(wxBitmap(im2));
  wxImage im3 =
      wxBitmap(now_blue).ConvertToImage().Scale(w, h, wxIMAGE_QUALITY_HIGH);
  m_bpNow->SetBitmap(wxBitmap(im3));
#endif

  this->Refresh();
}

void otcurrentUIDialog::SetViewPort(PlugIn_ViewPort* vp) {
  if (m_vp == vp) return;

  delete m_vp;
  m_vp = new PlugIn_ViewPort(*vp);
}

void otcurrentUIDialog::OnClose(wxCloseEvent& event) {
  m_FolderSelected = m_dirPicker1->GetValue();
  pPlugIn->m_CopyFolderSelected = m_FolderSelected;

  int i = m_choice1->GetSelection();
  m_IntervalSelected = i;
  pPlugIn->m_CopyIntervalSelected = m_IntervalSelected;

  pPlugIn->OnotcurrentDialogClose();
}

void otcurrentUIDialog::OpenFile(bool newestFile) {
  m_bUseRate = pPlugIn->GetCopyRate();
  m_bUseDirection = pPlugIn->GetCopyDirection();
  m_bUseHighRes = pPlugIn->GetCopyResolution();
  m_bUseFillColour = pPlugIn->GetCopyColour();
  m_arrow_scale = pPlugIn->GetCopyArrowScale() + 1;

  m_IntervalSelected = pPlugIn->GetIntervalSelected();
  if (m_FolderSelected == wxEmptyString) {
#ifndef __OCPN__ANDROID__
    m_FolderSelected = pPlugIn->GetFolderSelected();
    m_dirPicker1->SetValue(m_FolderSelected);
    wxDirDialog* d = new wxDirDialog(this, _("Choose the tcdata directory"), "",
                                     0, wxDefaultPosition);
    if (d->ShowModal() == wxID_OK) {
      m_dirPicker1->SetValue(d->GetPath());
      m_FolderSelected = m_dirPicker1->GetValue();
    }
#else
    wxString tc =
        "/storage/emulated/0/Android/data/org.opencpn.opencpn/files/tcdata";
    m_dirPicker1->SetValue(tc);
    m_FolderSelected = tc;

#endif
  }

  LoadTCMFile();
}

void otcurrentUIDialog::OnPreferences(wxCommandEvent& event) {
  pPlugIn->ShowPreferencesDialog(pParent);
}

void otcurrentUIDialog::OnSelectData(wxCommandEvent& event) {
#ifndef __OCPN__ANDROID__
  wxDirDialog directory_dialog(this, _("Choose a Tide/Current data directory"),
                               m_FolderSelected, 0, wxDefaultPosition);
  if (directory_dialog.ShowModal() == wxID_OK) {
    const wxString previous_folder = m_FolderSelected;
    m_FolderSelected = directory_dialog.GetPath();
    m_dirPicker1->SetValue(m_FolderSelected);
    pPlugIn->m_CopyFolderSelected = m_FolderSelected;
    if (previous_folder != m_FolderSelected) m_TcdSelectionSetting = _T("*");
  } else {
    return;
  }
#else
  wxString tc =
      "/storage/emulated/0/Android/data/org.opencpn.opencpn/files/tcdata";
  m_dirPicker1->SetValue(tc);
  m_FolderSelected = tc;
  pPlugIn->m_CopyFolderSelected = m_FolderSelected;
#endif

  wxArrayString tcd_files = FindTcdFiles(m_FolderSelected);
  if (tcd_files.GetCount() != 0) {
    wxMultiChoiceDialog file_dialog(
        this,
        _("Select one or more TCD files. Avoid overlapping regions unless "
          "duplicate arrows are intended."),
        _("otcurrent_LTC_V.2.6_R - TCD files"), tcd_files);

    wxArrayInt current_selections;
    for (unsigned int i = 0; i < tcd_files.GetCount(); i++) {
      if (SelectionContains(m_TcdSelectionSetting, tcd_files[i]))
        current_selections.Add(static_cast<int>(i));
    }
    file_dialog.SetSelections(current_selections);

    if (file_dialog.ShowModal() != wxID_OK) return;

    wxArrayInt selections = file_dialog.GetSelections();
    if (selections.GetCount() == 0) {
      m_TcdSelectionSetting = _T("!");
    } else {
      m_TcdSelectionSetting.Clear();
      for (unsigned int i = 0; i < selections.GetCount(); i++) {
        if (!m_TcdSelectionSetting.IsEmpty())
          m_TcdSelectionSetting.Append(_T(";"));
        m_TcdSelectionSetting.Append(tcd_files[selections[i]]);
      }
    }
  } else {
    m_TcdSelectionSetting = _T("!");
    wxMessageBox(
        _("No TCD files were found in the selected directory. Switch to "
          "OpenCPN IDX mode to use the IDX sources configured in OpenCPN."),
        _("otcurrent_LTC_V.2.6_R"), wxOK | wxICON_INFORMATION, this);
  }

  LoadTCMFile();
  LoadHarmonics();
  RequestRefresh(pParent);
}

void otcurrentUIDialog::OnSourceMode(wxCommandEvent& event) {
  m_SourceMode = m_sourceModeChoice->GetSelection();
  if (m_SourceMode < 0 || m_SourceMode > 2) m_SourceMode = 0;
  const bool tcd_mode = m_SourceMode != 0;
  m_dirPicker1->Enable(tcd_mode);
  m_button2->Enable(tcd_mode);

  wxFileConfig* config = GetOCPNConfigObject();
  if (config) {
    config->SetPath(_T("/PlugIns/otcurrent_ltc_pi"));
    config->Write(_T("otcurrentSourceMode"), m_SourceMode);
    config->Flush();
  }

  LoadTCMFile();
  LoadHarmonics();
  RequestRefresh(pParent);
}

void otcurrentUIDialog::OnSelectInterval(wxCommandEvent& event) {
  int i = m_choice1->GetSelection();
  m_IntervalSelected = i;
  pPlugIn->m_CopyIntervalSelected = m_IntervalSelected;
}
void otcurrentUIDialog::OnCalendarShow(wxCommandEvent& event) {
  CalendarDialog CalDialog(this, -1, _("START Date/Time"), wxPoint(100, 100),
                           wxSize(-1, -1));

#ifdef __OCPN__ANDROID__
  wxDateTime now = wxDateTime::Now();
  CalDialog.dialogCalendar->SetValue(now);
#endif

  if (CalDialog.ShowModal() == wxID_OK) {
    wxDateTime dm = CalDialog.dialogCalendar->GetDate();
    wxString myTime = CalDialog._timeText->GetValue();
    wxString val = myTime.Mid(0, 1);

    if (val == wxT(" ")) {
      myTime = wxT("0") + myTime.Mid(1, 5);
    }

    wxDateTime dt;
    dt.ParseTime(myTime);

    wxString todayHours = dt.Format(_T("%H"));
    wxString todayMinutes = dt.Format(_T("%M"));

    double h;
    double m;

    todayHours.ToDouble(&h);
    todayMinutes.ToDouble(&m);
    myTimeOfDay = wxTimeSpan(h, m, 0, 0);

    dm = CalDialog.dialogCalendar->GetDate();

    m_dtNow = dm + myTimeOfDay;

    MakeDateTimeLabel(m_dtNow);
    RequestRefresh(pParent);
  }
}

void otcurrentUIDialog::OnNow(wxCommandEvent& event) {
  m_dtNow = wxDateTime::Now();
  MakeDateTimeLabel(m_dtNow);

  RequestRefresh(pParent);

  onPrev = false;
  onNext = false;
}

void otcurrentUIDialog::OnPrev(wxCommandEvent& event) {
  int i = m_choice1->GetSelection();
  wxString c = m_choice1->GetString(i);
  double value;
  c.ToDouble(&value);
  m_dInterval = value;

  wxTimeSpan m_ts = wxTimeSpan::Minutes(m_dInterval);
  m_dtNow.Subtract(m_ts);
  MakeDateTimeLabel(m_dtNow);

  RequestRefresh(pParent);
}

void otcurrentUIDialog::OnNext(wxCommandEvent& event) {
  int i = m_choice1->GetSelection();
  wxString c = m_choice1->GetString(i);

  double value;
  c.ToDouble(&value);
  m_dInterval = value;

  wxTimeSpan m_ts = wxTimeSpan::Minutes(m_dInterval);
  m_dtNow.Add(m_ts);
  MakeDateTimeLabel(m_dtNow);

  RequestRefresh(pParent);
}

void otcurrentUIDialog::SetInterval(wxCommandEvent& event) {
  int i = m_choice1->GetSelection();
  wxString c = m_choice1->GetString(i);
  double value;
  c.ToDouble(&value);
  m_dInterval = value;
}

wxString otcurrentUIDialog::MakeDateTimeLabel(wxDateTime myDateTime) {
  wxDateTime dt = myDateTime;

  wxString s2 = dt.Format(_T( "%a %d %b %Y"));
  wxString s = dt.Format(_T("%H:%M"));
  wxString dateLabel = s2 + _T(" ") + s;

  m_textCtrl1->SetValue(dateLabel);

  return dateLabel;
}

void otcurrentUIDialog::About(wxCommandEvent& event) {
  wxMessageBox(_("Tidal "
                 "Current\n----------------------------------------------------"
                 "----------\n\n\n\n\n\nUse this data with caution.\nUse in "
                 "conjunction with Tidal Current Atlases and Tidal "
                 "Diamonds\n\n-------------------------------------------------"
                 "-------------------\n\nNote: 1 Rates shown are for a "
                 "position corresponding to the centre\nof the base of the "
                 "arrow. Tidal rate is shown as knots.\n\n"
                 "Original Plugin by Mike Rossiter\n"
                 "modified by Christian Streicher , Christian Streicher@ "
                 "facebook\n"),
               _("About Tidal Arrows"), wxOK | wxICON_INFORMATION, this);
}

CalendarDialog::CalendarDialog(wxWindow* parent, wxWindowID id,
                               const wxString& title, const wxPoint& position,
                               const wxSize& size, long style)
    : wxDialog(parent, id, title, position, size, style) {
  wxString dimensions = wxT(""), s;
  wxPoint p;
  wxSize sz;

  sz.SetWidth(200);
  sz.SetHeight(600);

  p.x = 6;
  p.y = 2;
  s.Printf(_(" x = %d y = %d\n"), p.x, p.y);
  dimensions.append(s);
  s.Printf(_(" width = %d height = %d\n"), sz.GetWidth(), sz.GetHeight());
  dimensions.append(s);
  dimensions.append(wxT("here"));

  itemBoxSizerFinal = new wxBoxSizer(wxVERTICAL);

#ifndef __OCPN__ANDROID__

  dialogCalendar = new wxCalendarCtrl(this, -1, wxDefaultDateTime,
                                      wxDefaultPosition, wxDefaultSize,
                                      wxCAL_SHOW_HOLIDAYS, _("Tide Calendar"));

#else
  m_staticTextDate = new wxStaticText(this, wxID_ANY, _("Date:"),
                                      wxDefaultPosition, wxDefaultSize);
  itemBoxSizerFinal->Add(m_staticTextDate, 0, wxEXPAND | wxALL, 10);
  dialogCalendar =
      new wxDatePickerCtrl(this, wxID_ANY, wxDefaultDateTime, wxDefaultPosition,
                           wxDefaultSize, wxDP_DEFAULT);
#endif

  itemBoxSizerFinal->Add(dialogCalendar, 0, wxEXPAND | wxALL, 10);

  itemBoxSizer1 = new wxBoxSizer(wxHORIZONTAL);
  m_staticText = new wxStaticText(this, wxID_ANY, _("Time:"), wxDefaultPosition,
                                  wxDefaultSize);
  _timeText = new wxTextCtrl(this, wxID_ANY, "12:00", wxDefaultPosition,
                             wxDefaultSize, 0);

  itemBoxSizer1->Add(m_staticText, 1, wxEXPAND | wxALL, 10);
  itemBoxSizer1->Add(_timeText, 1, wxEXPAND | wxALL, 10);

  itemBoxSizerFinal->Add(itemBoxSizer1, 0, wxEXPAND | wxALL, 10);

  itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);

  c = new wxButton(this, wxID_CANCEL, _("Cancel"), p, wxDefaultSize);
  b = new wxButton(this, wxID_OK, _("OK"), p, wxDefaultSize);

  itemBoxSizer2->Add(c, 1, wxEXPAND | wxALL, 10);
  itemBoxSizer2->Add(b, 1, wxEXPAND | wxALL, 10);

  itemBoxSizerFinal->Add(itemBoxSizer2, 0, wxEXPAND | wxALL, 10);

  this->SetSizer(itemBoxSizerFinal);
  this->Layout();
  itemBoxSizerFinal->Fit(this);
}
