;BASED ON UNPLUGRED'S RENDITION OF DISCODSP'S INNO SETUP SCRIPT

[Setup]
DisableDirPage=yes
ArchitecturesInstallIn64BitMode=x64
AppName={#PluginName}
AppPublisherURL=https://aviaryaudio.com/
AppSupportURL=https://aviaryaudio.com/
AppUpdatesURL=https://aviaryaudio.com/
AppVerName={#PluginName}
AppVersion=1.0
Compression=lzma2/ultra64
DefaultDirName={commonpf}\AviaryAudio\
DefaultGroupName=AviaryAudio
DisableReadyPage=false
DisableWelcomePage=no
LanguageDetectionMethod=uilanguage
OutputDir=./Output
OutputBaseFilename={#PluginName} Installer
SetupIconFile=assets\miniburger.ico
ShowLanguageDialog=no
VersionInfoCompany=AviaryAudio
VersionInfoCopyright=AviaryAudio
VersionInfoDescription={#PluginName}
VersionInfoProductName={#PluginName}
VersionInfoProductVersion=1.0
VersionInfoVersion=1.0
WizardImageFile=assets\Hamburger.bmp
WizardImageStretch=false
WizardSmallImageFile=assets\miniburger.bmp


[Files]
Source: "..\build_windows\{#PluginName}.vst3"; DestDir: "{commoncf64}\VST3\AviaryAudio\"; Components: VST3; Flags: ignoreversion
Source: "..\build_windows\{#PluginName}.clap"; DestDir: {code:GetDir|0}; Components: CLAP; Flags: ignoreversion
#ifdef Standalone
Source: "..\build_windows\{#PluginName}.exe"; DestDir: {code:GetDir|1}; Components: Standalone; Flags: ignoreversion
#endif
#ifdef OtherData
Source: "..\build_windows\other\{#PluginName}\*.*"; DestDir: "{commoncf64}\VST3\AviaryAudio\{#PluginName}"; Components: VST3; Flags: recursesubdirs onlyifdoesntexist
Source: "..\build_windows\other\{#PluginName}\*.*"; DestDir: "{code:GetDir|0}\{#PluginName}"; Components: CLAP; Flags: recursesubdirs onlyifdoesntexist
#ifdef Standalone
Source: "..\build_windows\other\{#PluginName}\*.*"; DestDir: "{code:GetDir|1}\{#PluginName}"; Components: Standalone; Flags: recursesubdirs onlyifdoesntexist
#endif
#endif

[Icons]
Name: {group}\Uninstall {#PluginName}; Filename: {uninstallexe}

[Types]
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "VST3"; Description: "VST3"; Types: custom;
Name: "CLAP"; Description: "CLAP"; Types: custom;
#ifdef Standalone
Name: "Standalone"; Description: "Standalone"; Types: custom;
#endif

[Code]
var
  DirPage: TInputDirWizardPage;
  TypesComboOnChangePrev: TNotifyEvent;

procedure ComponentsListCheckChanges;
begin
  WizardForm.NextButton.Enabled := (WizardSelectedComponents(False) <> '');
end;

procedure ComponentsListClickCheck(Sender: TObject);
begin
  ComponentsListCheckChanges;
end;

procedure TypesComboOnChange(Sender: TObject);
begin
  TypesComboOnChangePrev(Sender);
  ComponentsListCheckChanges;
end;

procedure InitializeWizard;
begin

  WizardForm.ComponentsList.OnClickCheck := @ComponentsListClickCheck;
  TypesComboOnChangePrev := WizardForm.TypesCombo.OnChange;
  WizardForm.TypesCombo.OnChange := @TypesComboOnChange;

  DirPage := CreateInputDirPage(wpSelectComponents,
  'Confirm Plugin Directory', '',
  'Select the folder in which setup should install the plugins, then click Next.',
  False, '');

  DirPage.Add('CLAP folder');
  DirPage.Values[0] := GetPreviousData('CLAP', ExpandConstant('{reg:HKLM\SOFTWARE\CLAP,CLAPPluginsPath|{commonpf}\Common Files\CLAP\AviaryAudio}'));
#ifdef Standalone
  DirPage.Add('Standalone folder');
  DirPage.Values[1] := GetPreviousData('Standalone', ExpandConstant('{reg:HKLM\SOFTWARE\CLAP,CLAPPluginsPath|{commonpf}\Common Files\CLAP\AviaryAudio}'));
#endif

end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = DirPage.ID then
  begin
    DirPage.Buttons[0].Enabled := WizardIsComponentSelected('CLAP');
    DirPage.PromptLabels[0].Enabled := DirPage.Buttons[0].Enabled;
    DirPage.Edits[0].Enabled := DirPage.Buttons[0].Enabled;

#ifdef Standalone
    DirPage.Buttons[1].Enabled := WizardIsComponentSelected('Standalone');
    DirPage.PromptLabels[1].Enabled := DirPage.Buttons[1].Enabled;
    DirPage.Edits[1].Enabled := DirPage.Buttons[1].Enabled;
#endif
  end;

  if CurPageID = wpSelectComponents then
  begin
    ComponentsListCheckChanges;
  end;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  if PageID = DirPage.ID then
  begin
#ifdef Standalone
    If (not WizardIsComponentSelected('CLAP')) and (not WizardIsComponentSelected('Standalone')) then
#else
    If (not WizardIsComponentSelected('CLAP')) then
#endif
      begin
        Result := True
      end;
  end;
end;

function GetDir(Param: string): string;
begin
    Result := DirPage.Values[StrToInt(Param)];
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  SetPreviousData(PreviousDataKey, 'CLAP', DirPage.Values[0]);
#ifdef Standalone
  SetPreviousData(PreviousDataKey, 'Standalone', DirPage.Values[1]);
#endif
end;