# otcurrent_LTC_V.2.20
modified opencpn plugin

this plugin based on "otcurrent v.4.6.5" from Mike Rossiter find here:
https://github.com/Rasbats/otcurrent_pi

reason for modification:
the original plugin only allow the hardcoded filenames harmonics-dwf-20210110-free.tcd and  HARMONIC.IDX
and more then one files are not possible.
hardcoded details:
 src\otcurrentUIDialogBase.cpp in line 219 and 220 you find something like this:
_______________________________
void otcurrentUIDialog::LoadTCMFile() {
wxString TCDir = m_FolderSelected;
TCDir.Append(wxFileName::GetPathSeparator());
wxLogMessage(_("Using Tide/Current data from: ") + TCDir);

wxString default_tcdata0 = TCDir + _T("harmonics-dwf-20210110-free.tcd");
wxString default_tcdata1 = TCDir + _T("HARMONIC.IDX");

// if (!TideCurrentDataSet.GetCount()) {
TideCurrentDataSet.Add(default_tcdata0);
TideCurrentDataSet.Add(default_tcdata1);
________________________________
now you can use all filenames and  multiple selection

<img width="1880" height="983" alt="nordsee" src="https://github.com/user-attachments/assets/75fe95a0-d8d5-4095-81b4-15bfac0c3ee1" />




