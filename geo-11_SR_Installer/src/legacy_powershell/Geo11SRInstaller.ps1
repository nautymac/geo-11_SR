# geo-11 SR 설치기 (GUI) - Unity / 일반 geo-11 패키지를 게임에 설치하고, 선택에 따라 설정 파일을 고친다.
# 실행: Geo11_SR_Installer.bat (STA PowerShell로 띄움). 패키지 폴더는 이 폴더 옆에 있어야 한다:
#   ..\geo-11 v0.6.109_Unity_Complete_SR\{x64,x32}
#   ..\geo-11 v0.6.109_Preferred_SR\{x64,x32}
# 고치는 파일: d3dxdm.ini (direct_mode, dm_separation, dm_convergence, dm_auto_convergence),
#   ShaderFixesDM\hotkeys.ini (Key = ...), d3dx.ini (hunting, toggle_hunting, reload_config, reload_fixes,
#   Unity include 줄은 UnitySwitch.ps1), SRWeave.ini (weave, swap_eyes, lens, key_*).
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$packages = [ordered]@{
    unity = @{ name = 'Unity (Unity Universal Fix + geo-11)'; dir = (Join-Path $root 'geo-11 v0.6.109_Unity_Complete_SR') }
    plain = @{ name = '일반 geo-11 (Preferred)';               dir = (Join-Path $root 'geo-11 v0.6.109_Preferred_SR') }
}
$unityVers = [ordered]@{ auto = '자동 감지'; '55' = '5.5 이하'; '56' = '5.6'; '2017' = '2017 - 2018'; '2019' = '2019 이상' }
$directModes = 'sbs', 'sbs_reversed', 'tab', 'tab_reversed', 'interlaced', 'interlaced_reversed', 'checkerboard', 'checkerboard_reversed', 'nvidia_dx11', 'nvidia_dx9', 'katanga_vr'
$enc = [Text.Encoding]::Default

# ---- key tables -------------------------------------------------------------------------------
# geo-11 keys live in ShaderFixesDM\hotkeys.ini as "[Section] / Key = ..." (3DMigoto syntax: ctrl alt shift + key name)
$geoKeys = @(
    @{ id = 'KeyToggleStereo';               label = '3D 켜기 / 끄기';            def = 'ctrl t' },
    @{ id = 'KeyIncreaseSeparation';         label = 'Separation(입체감) 증가';   def = 'ctrl F4' },
    @{ id = 'KeyDecreaseSeparation';         label = 'Separation(입체감) 감소';   def = 'ctrl F3' },
    @{ id = 'KeyIncreaseConvergence';        label = 'Convergence(수렴) 증가';    def = 'ctrl F6' },
    @{ id = 'KeyDecreaseConvergence';        label = 'Convergence(수렴) 감소';    def = 'ctrl F5' },
    @{ id = 'KeySaveSettings';               label = '현재 값 저장';              def = 'ctrl F7' },
    @{ id = 'KeyToggleOverlayStereoParams';  label = '입체 값 오버레이 표시';     def = 'ctrl F1' }
)
# 3DMigoto keys live in d3dx.ini [Hunting]
$migotoKeys = @(
    @{ id = 'toggle_hunting'; label = 'Hunting 켜기 / 끄기 (오버레이)'; def = 'no_modifiers NO_VK_DECIMAL VK_NUMPAD0' },
    @{ id = 'reload_config';  label = 'd3dx.ini 다시 읽기';            def = 'no_modifiers VK_F10' },
    @{ id = 'reload_fixes';   label = 'ShaderFixes 다시 읽기';          def = 'no_modifiers VK_F10' },
    @{ id = 'show_original';  label = '픽스 잠시 끄기 (누르는 동안)';   def = 'no_modifiers VK_F9' }
)
# SRWeave keys live in SRWeave.ini (always Ctrl+Alt + key)
$srKeys = @(
    @{ id = 'key_weave'; label = '위빙 켜기 / 끄기';        def = 'W' },
    @{ id = 'key_swap';  label = '좌우 눈 바꾸기';          def = 'S' },
    @{ id = 'key_lens';  label = '렌티큘러 렌즈 켜기 / 끄기'; def = 'L' },
    @{ id = 'key_test';  label = '테스트 무늬 (왼눈 빨강)';   def = 'T' }
)

# ---- helpers ----------------------------------------------------------------------------------

function Get-ExeBits($path) {
    $fs = [IO.File]::OpenRead($path)
    try {
        $br = New-Object IO.BinaryReader($fs)
        $fs.Position = 0x3C
        $fs.Position = $br.ReadInt32() + 4
        $machine = $br.ReadUInt16()
    } finally { $fs.Close() }
    if ($machine -eq 0x8664) { return 64 }
    if ($machine -eq 0x014C) { return 32 }
    return $null
}

function Get-UnityVersion($dir, $exe) {
    $dll = Join-Path $dir 'UnityPlayer.dll'
    if (Test-Path -LiteralPath $dll) {
        $v = (Get-Item -LiteralPath $dll).VersionInfo.FileVersion
        if ($v -match '^(\d+\.\d+\.\d+)') { return $Matches[1] }
    }
    $data = Join-Path $dir ((Get-Item -LiteralPath $exe).BaseName + '_Data')
    if (Test-Path -LiteralPath $data) {
        foreach ($n in 'globalgamemanagers', 'mainData', 'data.unity3d') {
            $f = Join-Path $data $n
            if (-not (Test-Path -LiteralPath $f)) { continue }
            $fs = [IO.File]::OpenRead($f)
            try { $buf = New-Object byte[] 4096; $len = $fs.Read($buf, 0, $buf.Length) } finally { $fs.Close() }
            $txt = [Text.Encoding]::ASCII.GetString($buf, 0, $len)
            if ($txt -match '(\d{1,4}\.\d+\.\d+)[abfp]\d+') { return $Matches[1] }
        }
        return 'unknown'
    }
    return $null
}

function Same-File($a, $b) {
    if (-not (Test-Path -LiteralPath $a) -or -not (Test-Path -LiteralPath $b)) { return $false }
    return (Get-FileHash -LiteralPath $a).Hash -eq (Get-FileHash -LiteralPath $b).Hash
}

# value of "name = value" (first non-comment match) or $null
function Get-IniValue($file, $name) {
    if (-not (Test-Path -LiteralPath $file)) { return $null }
    foreach ($l in [IO.File]::ReadAllLines($file, $enc)) {
        if ($l -match ('^\s*' + [regex]::Escape($name) + '\s*=\s*(.*?)\s*$')) { return $Matches[1] }
    }
    return $null
}

# replace every active "name = value" line; if none exists, append under [section] (or at the end)
function Set-IniValue($file, $name, $value, $section) {
    $lines = [IO.File]::ReadAllLines($file, $enc)
    $rx = '^\s*' + [regex]::Escape($name) + '\s*='
    $done = $false
    $out = foreach ($l in $lines) {
        if ($l -match $rx) { if (-not $done) { "$name = $value"; $done = $true } } else { $l }
    }
    if (-not $done) {
        $out2 = New-Object System.Collections.Generic.List[string]
        $added = $false
        foreach ($l in $out) {
            $out2.Add($l)
            if (-not $added -and $section -and $l -match ('^\s*\[' + [regex]::Escape($section) + '\]\s*$')) { $out2.Add("$name = $value"); $added = $true }
        }
        if (-not $added) { if ($section) { $out2.Add("[$section]") }; $out2.Add("$name = $value") }
        $out = $out2.ToArray()
    }
    [IO.File]::WriteAllLines($file, [string[]]$out, $enc)
}

# "Key = ..." inside [section] of a 3DMigoto ini
function Get-SectionKey($file, $section) {
    if (-not (Test-Path -LiteralPath $file)) { return $null }
    $in = $false
    foreach ($l in [IO.File]::ReadAllLines($file, $enc)) {
        if ($l -match '^\s*\[(.+?)\]\s*$') { $in = ($Matches[1] -ieq $section); continue }
        if ($in -and $l -match '^\s*Key\s*=\s*(.*?)\s*$') { return $Matches[1] }
    }
    return $null
}

function Set-SectionKey($file, $section, $value) {
    $lines = [IO.File]::ReadAllLines($file, $enc)
    $in = $false; $done = $false
    $out = foreach ($l in $lines) {
        if ($l -match '^\s*\[(.+?)\]\s*$') { $in = ($Matches[1] -ieq $section); $l; continue }
        if ($in -and -not $done -and $l -match '^\s*Key\s*=') { "Key = $value"; $done = $true } else { $l }
    }
    if (-not $done) { $out += "[$section]"; $out += "Key = $value" }
    [IO.File]::WriteAllLines($file, [string[]]$out, $enc)
}

# ---- form ---------------------------------------------------------------------------------------

$form = New-Object System.Windows.Forms.Form
$form.Text = 'geo-11 SR 설치기 - Unity / 일반 geo-11 + SR 디스플레이 위빙 (ReShade 불필요)'
$form.Size = New-Object System.Drawing.Size(900, 860)
$form.StartPosition = 'CenterScreen'
$form.MinimumSize = $form.Size
$form.Font = New-Object System.Drawing.Font('Malgun Gothic', 9)

$tip = New-Object System.Windows.Forms.ToolTip

function New-Label($text, $x, $y, $w, $parent, $bold) {
    $l = New-Object System.Windows.Forms.Label
    $l.Text = $text; $l.Location = New-Object System.Drawing.Point($x, $y); $l.AutoSize = $false
    $l.Size = New-Object System.Drawing.Size($w, 20)
    if ($bold) { $l.Font = New-Object System.Drawing.Font($form.Font, [System.Drawing.FontStyle]::Bold) }
    $parent.Controls.Add($l); return $l
}
function New-Group($text, $x, $y, $w, $h) {
    $g = New-Object System.Windows.Forms.GroupBox
    $g.Text = $text; $g.Location = New-Object System.Drawing.Point($x, $y); $g.Size = New-Object System.Drawing.Size($w, $h)
    $form.Controls.Add($g); return $g
}
function New-Radio($text, $x, $y, $w, $parent) {
    $r = New-Object System.Windows.Forms.RadioButton
    $r.Text = $text; $r.Location = New-Object System.Drawing.Point($x, $y); $r.Size = New-Object System.Drawing.Size($w, 22)
    $parent.Controls.Add($r); return $r
}
function New-Check($text, $x, $y, $w, $parent) {
    $c = New-Object System.Windows.Forms.CheckBox
    $c.Text = $text; $c.Location = New-Object System.Drawing.Point($x, $y); $c.Size = New-Object System.Drawing.Size($w, 22)
    $parent.Controls.Add($c); return $c
}
function New-Text($x, $y, $w, $parent) {
    $t = New-Object System.Windows.Forms.TextBox
    $t.Location = New-Object System.Drawing.Point($x, $y); $t.Size = New-Object System.Drawing.Size($w, 22)
    $parent.Controls.Add($t); return $t
}
function New-Button($text, $x, $y, $w, $parent) {
    $b = New-Object System.Windows.Forms.Button
    $b.Text = $text; $b.Location = New-Object System.Drawing.Point($x, $y); $b.Size = New-Object System.Drawing.Size($w, 28)
    $parent.Controls.Add($b); return $b
}

# -- 1. game ----------------------------------------------------------------------------------------
$gGame = New-Group '1. 게임 실행 파일' 12 10 860 95
New-Label '게임 .exe:' 12 28 70 $gGame | Out-Null
$txtExe = New-Text 85 25 660 $gGame
$btnBrowse = New-Button '찾아보기...' 755 23 90 $gGame
$lblDetect = New-Label '실행 파일을 고르면 32/64비트, Unity 여부, 기존 픽스, ReShade dxgi.dll을 자동으로 확인합니다.' 12 58 830 $gGame
$lblDetect.Size = New-Object System.Drawing.Size(830, 30)

# -- 2. package ---------------------------------------------------------------------------------------
$gPkg = New-Group '2. 패키지 / 비트 / 설치 범위' 12 110 430 150
New-Label '엔진:' 12 25 50 $gPkg | Out-Null
$rbUnity = New-Radio 'Unity' 65 23 70 $gPkg
$rbPlain = New-Radio '일반 geo-11' 140 23 120 $gPkg
$rbPlain.Checked = $true
New-Label 'Unity 버전:' 12 53 80 $gPkg | Out-Null
$cbUnityVer = New-Object System.Windows.Forms.ComboBox
$cbUnityVer.Location = New-Object System.Drawing.Point(95, 50); $cbUnityVer.Size = New-Object System.Drawing.Size(150, 22)
$cbUnityVer.DropDownStyle = 'DropDownList'
foreach ($k in $unityVers.Keys) { [void]$cbUnityVer.Items.Add($unityVers[$k]) }
$cbUnityVer.SelectedIndex = 0
$gPkg.Controls.Add($cbUnityVer)
$lblUnityDetected = New-Label '' 255 53 170 $gPkg
New-Label '비트:' 12 83 50 $gPkg | Out-Null
$rb64 = New-Radio '64비트 (x64)' 65 81 110 $gPkg
$rb32 = New-Radio '32비트 (x32)' 180 81 110 $gPkg
$rb64.Checked = $true
New-Label '범위:' 12 113 50 $gPkg | Out-Null
$rbFull = New-Radio '전체 설치 (geo-11 + 픽스 + SR)' 65 111 220 $gPkg
$rbSrOnly = New-Radio 'SR 위빙만 추가 (기존 픽스 유지)' 65 129 230 $gPkg
$rbFull.Checked = $true
$tip.SetToolTip($rbSrOnly, '게임 폴더에 이미 geo-11/3DMigoto d3d11.dll이 있을 때: dxgi.dll + SRWeave.ini만 넣고 d3dx.ini/ShaderFixes는 건드리지 않습니다.')

# -- 3. SR ----------------------------------------------------------------------------------------------
$gSr = New-Group '3. SR 디스플레이 (SRWeave dxgi.dll, SR SDK 위빙)' 452 110 420 150
$chkSr = New-Check 'SR 위빙 사용 (렌티큘러 패널용, ReShade 없이)' 12 22 380 $gSr
$chkSr.Checked = $true
$chkSwap = New-Check '좌우 눈 바꾸기 (입체가 뒤집혀 보일 때)' 30 46 350 $gSr
$chkLens = New-Check '위빙 중 렌티큘러 렌즈 켜기' 30 68 350 $gSr
$chkLens.Checked = $true
New-Label '3D 출력 모드 (direct_mode):' 12 98 180 $gSr | Out-Null
$cbMode = New-Object System.Windows.Forms.ComboBox
$cbMode.Location = New-Object System.Drawing.Point(195, 95); $cbMode.Size = New-Object System.Drawing.Size(150, 22)
$cbMode.DropDownStyle = 'DropDownList'
foreach ($m in $directModes) { [void]$cbMode.Items.Add($m) }
$cbMode.SelectedIndex = 0
$gSr.Controls.Add($cbMode)
$lblModeNote = New-Label 'SR 위빙은 sbs 입력이 필요해서 sbs로 고정됩니다.' 12 122 400 $gSr
$lblModeNote.ForeColor = [System.Drawing.Color]::DimGray

# -- 4. stereo values ----------------------------------------------------------------------------------
$gStereo = New-Group '4. 입체 값 (d3dxdm.ini [Stereo])' 12 265 430 90
New-Label 'Separation (입체감, 0-100):' 12 27 170 $gStereo | Out-Null
$numSep = New-Object System.Windows.Forms.NumericUpDown
$numSep.Location = New-Object System.Drawing.Point(190, 24); $numSep.Size = New-Object System.Drawing.Size(80, 22)
$numSep.Minimum = 0; $numSep.Maximum = 100; $numSep.Value = 80
$gStereo.Controls.Add($numSep)
New-Label 'Convergence (수렴, 0.1-8):' 12 55 170 $gStereo | Out-Null
$numConv = New-Object System.Windows.Forms.NumericUpDown
$numConv.Location = New-Object System.Drawing.Point(190, 52); $numConv.Size = New-Object System.Drawing.Size(80, 22)
$numConv.Minimum = 0.1; $numConv.Maximum = 8; $numConv.DecimalPlaces = 2; $numConv.Increment = 0.1; $numConv.Value = 3.0
$gStereo.Controls.Add($numConv)
$chkAutoConv = New-Check '자동 수렴 (dm_auto_convergence)' 285 25 140 $gStereo
$chkAutoConv.Size = New-Object System.Drawing.Size(140, 40)
$tip.SetToolTip($numSep, 'dm_separation: 게임 안에서 Separation 증가/감소 키로도 바꾸고 저장 키로 저장할 수 있습니다.')
$tip.SetToolTip($numConv, 'dm_convergence: 게임 안에서 Convergence 증가/감소 키로도 바꾸고 저장 키로 저장할 수 있습니다.')

# -- 5. hotkeys ---------------------------------------------------------------------------------------
$gKeys = New-Group '5. 단축키 (게임 안에서 사용, 바꿀 수 있음)' 452 265 420 330
$keyBoxes = @{}
$y = 22
New-Label 'geo-11 (ShaderFixesDM\hotkeys.ini) - 표기: ctrl / alt / shift + 키 이름' 12 $y 400 $gKeys $true | Out-Null
$y += 22
foreach ($k in $geoKeys) {
    New-Label $k.label 12 ($y + 2) 190 $gKeys | Out-Null
    $keyBoxes[$k.id] = New-Text 205 $y 200 $gKeys
    $keyBoxes[$k.id].Text = $k.def
    $y += 24
}
$y += 4
New-Label '3DMigoto (d3dx.ini [Hunting]) - 표기: no_modifiers / ctrl / alt / shift + VK_ 이름' 12 $y 400 $gKeys $true | Out-Null
$y += 22
New-Label 'Hunting 모드:' 12 ($y + 2) 100 $gKeys | Out-Null
$cbHunting = New-Object System.Windows.Forms.ComboBox
$cbHunting.Location = New-Object System.Drawing.Point(205, $y); $cbHunting.Size = New-Object System.Drawing.Size(200, 22)
$cbHunting.DropDownStyle = 'DropDownList'
[void]$cbHunting.Items.AddRange(@('0 - 끔 (가장 빠름)', '1 - 켬 (Hunting 키로 켜고 끔)', '2 - 꺼진 채 시작 (Hunting 키로 켬)'))
$cbHunting.SelectedIndex = 1
$gKeys.Controls.Add($cbHunting)
$y += 24
foreach ($k in $migotoKeys) {
    New-Label $k.label 12 ($y + 2) 190 $gKeys | Out-Null
    $keyBoxes[$k.id] = New-Text 205 $y 200 $gKeys
    $keyBoxes[$k.id].Text = $k.def
    $y += 24
}
$tip.SetToolTip($keyBoxes['toggle_hunting'], '기본값은 숫자패드 0 (NumLock 켜진 상태). NO_VK_DECIMAL은 숫자패드 .(Del)과 구분하기 위한 것입니다.')

$gKeys2 = New-Group '6. SRWeave 단축키 (SRWeave.ini) - 항상 Ctrl+Alt + 키' 12 360 430 150
$y = 22
foreach ($k in $srKeys) {
    New-Label ('Ctrl+Alt + ... : ' + $k.label) 12 ($y + 2) 230 $gKeys2 | Out-Null
    $keyBoxes[$k.id] = New-Text 250 $y 150 $gKeys2
    $keyBoxes[$k.id].Text = $k.def
    $y += 24
}
$lblSrKeyNote = New-Label '키: 글자/숫자(W, 5), F1-F24, NUMPAD0-9, VK_F6 같은 VK_ 이름, none' 12 ($y + 2) 410 $gKeys2
$lblSrKeyNote.ForeColor = [System.Drawing.Color]::DimGray
$btnDefaults = New-Button '단축키 기본값으로' 12 ($y + 24) 140 $gKeys2

# -- help ---------------------------------------------------------------------------------------------
$gHelp = New-Group '키 안내' 12 515 430 80
$lblHelp = New-Label ('geo-11: Ctrl+T 3D 켜기/끄기, Ctrl+F3/F4 Separation, Ctrl+F5/F6 Convergence, Ctrl+F7 저장, Ctrl+F1 값 표시' +
    [Environment]::NewLine + '3DMigoto: 숫자패드 0 Hunting 켜기/끄기, F10 설정 다시 읽기, F9 픽스 잠시 끄기' +
    [Environment]::NewLine + 'SRWeave: Ctrl+Alt+W 위빙, Ctrl+Alt+S 좌우, Ctrl+Alt+L 렌즈, Ctrl+Alt+T 테스트 (위 표에서 바꾸면 그 키)') 12 20 410 $gHelp
$lblHelp.Size = New-Object System.Drawing.Size(410, 55)

# -- actions / log ------------------------------------------------------------------------------------
$btnApply = New-Button '설치 / 적용' 12 605 150 $form
$btnApply.Font = New-Object System.Drawing.Font($form.Font, [System.Drawing.FontStyle]::Bold)
$btnRemoveSr = New-Button 'SR 위빙만 제거' 172 605 130 $form
$btnUninstall = New-Button '전체 제거 (Uninstall.bat)' 312 605 170 $form
$btnOpenDir = New-Button '게임 폴더 열기' 492 605 120 $form
$btnLog = New-Button 'SRWeave.log 보기' 622 605 130 $form
$txtLog = New-Object System.Windows.Forms.TextBox
$txtLog.Location = New-Object System.Drawing.Point(12, 640); $txtLog.Size = New-Object System.Drawing.Size(860, 170)
$txtLog.Multiline = $true; $txtLog.ScrollBars = 'Vertical'; $txtLog.ReadOnly = $true
$txtLog.Font = New-Object System.Drawing.Font('Consolas', 9)
$form.Controls.Add($txtLog)

function Log($msg) { $txtLog.AppendText($msg + [Environment]::NewLine) }

# ---- state ----------------------------------------------------------------------------------------
$script:gameDir = $null
$script:gameExe = $null

function Update-Enabled {
    $cbUnityVer.Enabled = $rbUnity.Checked
    $cbMode.Enabled = -not $chkSr.Checked
    if ($chkSr.Checked) { $cbMode.SelectedItem = 'sbs' }
    $lblModeNote.Visible = $chkSr.Checked
    $chkSwap.Enabled = $chkSr.Checked; $chkLens.Enabled = $chkSr.Checked
    foreach ($k in $srKeys) { $keyBoxes[$k.id].Enabled = $chkSr.Checked }
}
$rbUnity.add_CheckedChanged({ Update-Enabled })
$chkSr.add_CheckedChanged({ Update-Enabled })

function Get-SrcDir {
    $pkg = $(if ($rbUnity.Checked) { 'unity' } else { 'plain' })
    $arch = $(if ($rb64.Checked) { 'x64' } else { 'x32' })
    return (Join-Path $packages[$pkg].dir $arch)
}

# read the values an installed game already has (so re-running the installer edits instead of resetting)
function Load-Existing($dir) {
    $dm = Join-Path $dir 'd3dxdm.ini'
    $v = Get-IniValue $dm 'direct_mode';         if ($v -and $directModes -contains $v) { $cbMode.SelectedItem = $v }
    $v = Get-IniValue $dm 'dm_separation';       if ($v -match '^\d+(\.\d+)?$') { $numSep.Value = [math]::Min(100, [math]::Max(0, [decimal]$v)) }
    $v = Get-IniValue $dm 'dm_convergence';      if ($v -match '^\d+(\.\d+)?$') { $numConv.Value = [math]::Min(8, [math]::Max(0.1, [decimal]$v)) }
    $v = Get-IniValue $dm 'dm_auto_convergence'; if ($v -ne $null) { $chkAutoConv.Checked = ($v -ne '0') }
    $hk = Join-Path $dir 'ShaderFixesDM\hotkeys.ini'
    foreach ($k in $geoKeys) { $v = Get-SectionKey $hk $k.id; if ($v) { $keyBoxes[$k.id].Text = $v } }
    $dx = Join-Path $dir 'd3dx.ini'
    $v = Get-IniValue $dx 'hunting'; if ($v -match '^[012]$') { $cbHunting.SelectedIndex = [int]$v }
    foreach ($k in $migotoKeys) { $v = Get-IniValue $dx $k.id; if ($v) { $keyBoxes[$k.id].Text = $v } }
    $sr = Join-Path $dir 'SRWeave.ini'
    if (Test-Path -LiteralPath $sr) {
        $v = Get-IniValue $sr 'swap_eyes'; if ($v -ne $null) { $chkSwap.Checked = ($v -ne '0') }
        $v = Get-IniValue $sr 'lens';      if ($v -ne $null) { $chkLens.Checked = ($v -ne '0') }
        foreach ($k in $srKeys) { $v = Get-IniValue $sr $k.id; if ($v) { $keyBoxes[$k.id].Text = $v } }
    }
}

function Detect-Game($exePath) {
    if (-not (Test-Path -LiteralPath $exePath -PathType Leaf)) { $lblDetect.Text = '파일이 없습니다: ' + $exePath; return }
    $script:gameExe = $exePath
    $script:gameDir = Split-Path -Parent $exePath
    $bits = Get-ExeBits $exePath
    if ($bits -eq 32) { $rb32.Checked = $true } elseif ($bits -eq 64) { $rb64.Checked = $true }
    $uv = Get-UnityVersion $script:gameDir $exePath
    $notes = @()
    if ($bits) { $notes += "$bits비트" } else { $notes += '비트 알 수 없음' }
    if ($uv) {
        $rbUnity.Checked = $true
        $notes += "Unity $uv"
        $lblUnityDetected.Text = "감지: $uv"
        $cbUnityVer.SelectedIndex = 0
    } else {
        $rbPlain.Checked = $true
        $lblUnityDetected.Text = ''
        $notes += 'Unity 아님 (일반 geo-11)'
    }
    $hasFix = Test-Path -LiteralPath (Join-Path $script:gameDir 'd3d11.dll')
    if ($hasFix) {
        $notes += '기존 d3d11.dll(geo-11/3DMigoto) 있음 -> "SR 위빙만 추가" 권장'
        $rbSrOnly.Checked = $true
        Load-Existing $script:gameDir
    } else {
        $rbFull.Checked = $true
    }
    $dx = Join-Path $script:gameDir 'dxgi.dll'
    if (Test-Path -LiteralPath $dx) {
        if (Same-File $dx (Join-Path (Get-SrcDir) 'dxgi.dll')) { $notes += 'SRWeave dxgi.dll 이미 설치됨' }
        else { $notes += '다른 dxgi.dll(ReShade?) 있음 -> 설치 시 dxgi.dll.reshade_bak으로 백업' }
    }
    $lblDetect.Text = ($notes -join ' | ')
    Update-Enabled
    Log ("게임: " + $exePath)
    Log ("  " + ($notes -join ' | '))
}

$btnBrowse.add_Click({
    $dlg = New-Object System.Windows.Forms.OpenFileDialog
    $dlg.Filter = '실행 파일 (*.exe)|*.exe'
    $dlg.Title = '게임 실행 파일 선택'
    if ($dlg.ShowDialog() -eq 'OK') { $txtExe.Text = $dlg.FileName; Detect-Game $dlg.FileName }
})
$txtExe.add_Leave({ if ($txtExe.Text -and $txtExe.Text -ne $script:gameExe) { Detect-Game $txtExe.Text.Trim('"') } })
$form.AllowDrop = $true
$form.add_DragEnter({ param($s, $e) if ($e.Data.GetDataPresent([Windows.Forms.DataFormats]::FileDrop)) { $e.Effect = 'Copy' } })
$form.add_DragDrop({ param($s, $e)
    $f = @($e.Data.GetData([Windows.Forms.DataFormats]::FileDrop))[0]
    if (Test-Path -LiteralPath $f -PathType Container) { $c = @(Get-ChildItem -LiteralPath $f -Filter *.exe -File); if ($c.Count -ge 1) { $f = $c[0].FullName } }
    $txtExe.Text = $f; Detect-Game $f
})

$btnDefaults.add_Click({
    foreach ($k in $geoKeys + $migotoKeys + $srKeys) { $keyBoxes[$k.id].Text = $k.def }
    $cbHunting.SelectedIndex = 1
})

# ---- apply ----------------------------------------------------------------------------------------

function Require-Game {
    if (-not $script:gameDir) { [System.Windows.Forms.MessageBox]::Show('먼저 게임 실행 파일을 고르세요.', 'geo-11 SR 설치기') | Out-Null; return $false }
    return $true
}

function Backup-Reshade($dir, $ourDll) {
    $dx = Join-Path $dir 'dxgi.dll'
    if ((Test-Path -LiteralPath $dx) -and -not (Same-File $dx $ourDll)) {
        $bak = Join-Path $dir 'dxgi.dll.reshade_bak'
        if (Test-Path -LiteralPath $bak) { Remove-Item -LiteralPath $dx -Force; Log '  기존 dxgi.dll 삭제 (dxgi.dll.reshade_bak이 이미 있음)' }
        else { Move-Item -LiteralPath $dx -Destination $bak; Log '  기존 dxgi.dll(ReShade?) -> dxgi.dll.reshade_bak' }
    }
}

function Remove-SrFiles($dir, $ourDll, $restore) {
    $dx = Join-Path $dir 'dxgi.dll'
    if (Same-File $dx $ourDll) { Remove-Item -LiteralPath $dx -Force; Log '  SRWeave dxgi.dll 삭제' }
    foreach ($n in 'SRWeave.ini', 'SRWeave.log') { $p = Join-Path $dir $n; if (Test-Path -LiteralPath $p) { Remove-Item -LiteralPath $p -Force; Log "  $n 삭제" } }
    $p = Join-Path $dir 'ShaderFixes\SRWeave'; if (Test-Path -LiteralPath $p) { Remove-Item -LiteralPath $p -Recurse -Force; Log '  ShaderFixes\SRWeave 삭제' }
    $bak = Join-Path $dir 'dxgi.dll.reshade_bak'
    if ($restore -and (Test-Path -LiteralPath $bak) -and -not (Test-Path -LiteralPath $dx)) { Move-Item -LiteralPath $bak -Destination $dx; Log '  dxgi.dll.reshade_bak -> dxgi.dll 복원 (ReShade)' }
}

$btnApply.add_Click({
    if (-not (Require-Game)) { return }
    try {
        $dir = $script:gameDir
        $src = Get-SrcDir
        $ourDll = Join-Path $src 'dxgi.dll'
        if (-not (Test-Path -LiteralPath $ourDll)) { throw "패키지가 없습니다: $src (이 설치기 폴더 옆에 패키지 폴더가 있어야 합니다)" }
        $useSr = $chkSr.Checked
        Log '----------------------------------------'
        Log ("적용: " + $(if ($rbUnity.Checked) { 'Unity' } else { '일반 geo-11' }) + ' / ' + $(if ($rb64.Checked) { 'x64' } else { 'x32' }) + ' / ' +
            $(if ($rbFull.Checked) { '전체 설치' } else { 'SR만 추가' }) + ' / SR ' + $(if ($useSr) { '사용' } else { '사용 안 함' }))
        Log "  원본: $src"

        # files
        if ($useSr) { Backup-Reshade $dir $ourDll }
        if ($rbFull.Checked) {
            Get-ChildItem -LiteralPath $src | ForEach-Object {
                if (-not $useSr -and ($_.Name -ieq 'dxgi.dll' -or $_.Name -ieq 'SRWeave.ini')) { return }
                Copy-Item -LiteralPath $_.FullName -Destination $dir -Recurse -Force
            }
            if (-not $useSr) { $p = Join-Path $dir 'ShaderFixes\SRWeave'; if (Test-Path -LiteralPath $p) { Remove-Item -LiteralPath $p -Recurse -Force } }
            Log '  패키지 파일 복사 완료'
        } elseif ($useSr) {
            Copy-Item -LiteralPath $ourDll -Destination $dir -Force
            if (-not (Test-Path -LiteralPath (Join-Path $dir 'SRWeave.ini'))) { Copy-Item -LiteralPath (Join-Path $src 'SRWeave.ini') -Destination $dir }
            Log '  dxgi.dll + SRWeave.ini 복사 (기존 픽스 유지)'
        }
        if (-not $useSr) { Remove-SrFiles $dir $ourDll $true }

        # d3dxdm.ini
        $dm = Join-Path $dir 'd3dxdm.ini'
        if (Test-Path -LiteralPath $dm) {
            $mode = $(if ($useSr) { 'sbs' } else { [string]$cbMode.SelectedItem })
            Set-IniValue $dm 'direct_mode' $mode 'Device'
            Set-IniValue $dm 'dm_separation' ([string][int]$numSep.Value) 'Stereo'
            Set-IniValue $dm 'dm_convergence' ($numConv.Value.ToString('0.0#', [Globalization.CultureInfo]::InvariantCulture)) 'Stereo'
            Set-IniValue $dm 'dm_auto_convergence' $(if ($chkAutoConv.Checked) { '1' } else { '0' }) 'Stereo'
            Log "  d3dxdm.ini: direct_mode=$mode dm_separation=$([int]$numSep.Value) dm_convergence=$($numConv.Value) dm_auto_convergence=$([int]$chkAutoConv.Checked)"
        } else {
            Log '  경고: d3dxdm.ini 없음 - geo-11이 아닌 3DMigoto 픽스입니다. SRWeave는 geo-11 sbs 출력이 필요합니다.'
        }

        # hotkeys.ini
        $hk = Join-Path $dir 'ShaderFixesDM\hotkeys.ini'
        if (Test-Path -LiteralPath $hk) {
            foreach ($k in $geoKeys) { $v = $keyBoxes[$k.id].Text.Trim(); if ($v) { Set-SectionKey $hk $k.id $v } }
            Log '  hotkeys.ini: geo-11 단축키 적용'
        }

        # d3dx.ini
        $dx = Join-Path $dir 'd3dx.ini'
        if (Test-Path -LiteralPath $dx) {
            Set-IniValue $dx 'hunting' ([string]$cbHunting.SelectedIndex) 'Hunting'
            foreach ($k in $migotoKeys) { $v = $keyBoxes[$k.id].Text.Trim(); if ($v) { Set-IniValue $dx $k.id $v 'Hunting' } }
            Log "  d3dx.ini: hunting=$($cbHunting.SelectedIndex), toggle_hunting=$($keyBoxes['toggle_hunting'].Text)"
        }

        # Unity include line
        $us = Join-Path $dir 'UnitySwitch.ps1'
        if ($rbUnity.Checked -and (Test-Path -LiteralPath $us)) {
            $code = @($unityVers.Keys)[$cbUnityVer.SelectedIndex]
            $outp = & powershell -NoProfile -ExecutionPolicy Bypass -File $us $code 2>&1
            foreach ($l in $outp) { Log "  UnitySwitch: $l" }
        } elseif ($rbUnity.Checked) {
            Log '  UnitySwitch.ps1 없음 - Unity 버전 include 줄은 바꾸지 않았습니다.'
        }

        # SRWeave.ini
        $sr = Join-Path $dir 'SRWeave.ini'
        if ($useSr -and (Test-Path -LiteralPath $sr)) {
            Set-IniValue $sr 'weave' '1' 'SRWeave'
            Set-IniValue $sr 'swap_eyes' $(if ($chkSwap.Checked) { '1' } else { '0' }) 'SRWeave'
            Set-IniValue $sr 'lens' $(if ($chkLens.Checked) { '1' } else { '0' }) 'SRWeave'
            foreach ($k in $srKeys) { $v = $keyBoxes[$k.id].Text.Trim(); if (-not $v) { $v = 'none' }; Set-IniValue $sr $k.id $v 'SRWeave' }
            Log "  SRWeave.ini: swap_eyes=$([int]$chkSwap.Checked) lens=$([int]$chkLens.Checked) keys=" + (($srKeys | ForEach-Object { $keyBoxes[$_.id].Text }) -join '/')
        }
        # note: SRWeave.ini lines are "name=value" in the package; Set-IniValue writes "name = value", which GetPrivateProfile* reads the same way.

        foreach ($c in 'ShaderCache', 'ShaderCacheDM') { $p = Join-Path $dir $c; if (Test-Path -LiteralPath $p) { Remove-Item -LiteralPath $p -Recurse -Force; Log "  $c 비움" } }
        Log '완료. 게임을 SR 패널에서 패널 해상도로 전체화면/테두리 없는 창으로 실행하세요.'
        [System.Windows.Forms.MessageBox]::Show('적용했습니다.', 'geo-11 SR 설치기') | Out-Null
    } catch {
        Log ('오류: ' + $_.Exception.Message)
        [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, '오류', 'OK', 'Error') | Out-Null
    }
})

$btnRemoveSr.add_Click({
    if (-not (Require-Game)) { return }
    try {
        Log '----------------------------------------'
        Log 'SR 위빙 제거'
        Remove-SrFiles $script:gameDir (Join-Path (Get-SrcDir) 'dxgi.dll') $true
        Log '완료 (geo-11은 그대로입니다).'
    } catch { Log ('오류: ' + $_.Exception.Message) }
})

$btnUninstall.add_Click({
    if (-not (Require-Game)) { return }
    $un = Join-Path $script:gameDir 'Uninstall.bat'
    if (-not (Test-Path -LiteralPath $un)) { Log 'Uninstall.bat이 게임 폴더에 없습니다.'; return }
    $r = [System.Windows.Forms.MessageBox]::Show("게임 폴더의 Uninstall.bat을 실행해 geo-11, ShaderFixes, SRWeave를 모두 지웁니다.`n$($script:gameDir)`n계속할까요?", '전체 제거', 'YesNo', 'Warning')
    if ($r -ne 'Yes') { return }
    try {
        Log '----------------------------------------'
        Remove-SrFiles $script:gameDir (Join-Path (Get-SrcDir) 'dxgi.dll') $false
        Start-Process -FilePath 'cmd.exe' -ArgumentList '/c', "`"$un`"" -WorkingDirectory $script:gameDir -Wait -WindowStyle Hidden
        $bak = Join-Path $script:gameDir 'dxgi.dll.reshade_bak'
        if (Test-Path -LiteralPath $bak) { Move-Item -LiteralPath $bak -Destination (Join-Path $script:gameDir 'dxgi.dll') -Force; Log '  dxgi.dll.reshade_bak -> dxgi.dll 복원' }
        Log 'Uninstall.bat 실행 완료.'
    } catch { Log ('오류: ' + $_.Exception.Message) }
})

$btnOpenDir.add_Click({ if (Require-Game) { Start-Process explorer.exe $script:gameDir } })
$btnLog.add_Click({
    if (-not (Require-Game)) { return }
    $l = Join-Path $script:gameDir 'SRWeave.log'
    if (Test-Path -LiteralPath $l) { Start-Process notepad.exe "`"$l`"" } else { Log 'SRWeave.log가 아직 없습니다 (게임을 한 번 실행하면 생깁니다).' }
})

# packages present?
foreach ($k in $packages.Keys) {
    $d = $packages[$k].dir
    if (Test-Path -LiteralPath $d) { Log ("패키지 확인: " + $packages[$k].name + " -> " + $d) }
    else { Log ("패키지 없음: " + $packages[$k].name + " -> " + $d) }
}
Update-Enabled
[void]$form.ShowDialog()
