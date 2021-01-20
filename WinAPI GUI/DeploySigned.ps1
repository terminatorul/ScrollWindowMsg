$BuildDirectory = Get-Location
$DeployDirectory = $args[1]
$AlternateDeployDirectory = $args[2]

$Windows_SDK_PlatformVersion = Get-ChildItem (${Env:ProgramFiles(x86)} + '\Windows Kits') | Sort-Object -Property @{Expression = {[System.Version] ($_.Name + '.0')}} -Descending | Select -First 1 -ExpandProperty Name
$Windows_SDK_Version = Get-ChildItem (${Env:ProgramFiles(x86)} + '\Windows Kits\' + $Windows_SDK_PlatformVersion + '\bin') | Where-Object -Property Name -CMatch '^[0-9.]+$' | Sort -Property @{Expression = {[System.Version] $_.Name}} -Descending | Select-Object -ExpandProperty Name -First 1
$SignToolExe = ${Env:ProgramFiles(x86)} + '\Windows Kits\' + $Windows_SDK_PlatformVersion + '\bin\' + $Windows_SDK_Version + '\x64\signtool.exe'

Start-Process  `
	-Verb RunAs `
	$env:ComSpec `
	-ArgumentList `
		'/D','/C',`
		(
			'XCopy /D /Y /F "' + $BuildDirectory + '\*.exe" "' + $DeployDirectory + '\Win-UI-Commands\\"' +
				' & ' +
			'(' +
				'For %f In ("' + $DeployDirectory + '\Win-UI-Commands\*.exe") Do ' +
				'@(' +
						'@"' + $SignToolExe + '" verify /pa /q "%~f" >NUL' +
							' & ' +
						'@If ErrorLevel 1 ' +
							'"' + $SignToolExe + '" sign /a /fd SHA512 /d "Windows UI Commands" "%~f"' +
				')' +
			')' +
				' & ' +
			'(If Exist "' + $AlternateDeployDirectory + '" XCopy /D /F /Y /I "' + $DeployDirectory + '\Win-UI-Commands\*.exe" "' + $AlternateDeployDirectory + '\Win-UI-Commands\\")' +
				' & ' +
			'Pause' `
		)