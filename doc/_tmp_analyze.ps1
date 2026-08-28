param([string]$Path = 'e:/automaton/build/retina_el5.log')
$log = Get-Content $Path
Write-Output ('log: ' + $Path + '  lines: ' + $log.Count)
Write-Output '== retina lines =='
$log | Where-Object { $_ -match 'retina tick' } | ForEach-Object { Write-Output $_ }
Write-Output '== closure lines =='
$log | Where-Object { $_ -match 'closure tick' } | ForEach-Object { Write-Output $_ }
Write-Output '== last 6 lines =='
$log | Select-Object -Last 6 | ForEach-Object { Write-Output $_ }