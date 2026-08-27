param(
    [string]$Csv = "build\ts_el7.csv"
)

$rows = Import-Csv $Csv
$frames = ($rows | Measure-Object -Property frame -Maximum).Maximum
$islands = ($rows | Measure-Object -Property island -Maximum).Maximum

function Fit($pts) {
    $n = $pts.Count
    if ($n -lt 8) { return $null }
    $mx = ($pts | ForEach-Object { $_.x } | Measure-Object -Average).Average
    $my = ($pts | ForEach-Object { $_.y } | Measure-Object -Average).Average
    $sxx = 0.0; $sxy = 0.0; $syy = 0.0
    foreach ($p in $pts) {
        $dx = $p.x - $mx; $dy = $p.y - $my
        $sxx += $dx * $dx; $sxy += $dx * $dy; $syy += $dy * $dy
    }
    if ($sxx -le 0) { return $null }
    $slope = $sxy / $sxx
    $inter = $my - $slope * $mx
    $sse = $syy - $slope * $sxy
    $r2 = 0.0; if ($syy -gt 0) { $r2 = [Math]::Max(0.0, 1.0 - $sse / $syy) }
    $se = 0.0
    if ($n -gt 2 -and $sse -gt 0) { $se = [Math]::Sqrt(($sse / ($n - 2)) / $sxx) }
    [pscustomobject]@{ slope = $slope; inter = $inter; r2 = $r2; se = $se; n = $n }
}

# Pooled regression across all islands: (N_t, N_{t+1}-N_t)
$pooled = New-Object System.Collections.Generic.List[object]
$perIsland = @()
$totalCap = 0.0; $totalEsc = 0.0; $totalN = 0.0; $totalSamples = 0

for ($g = 0; $g -le $islands; $g++) {
    $ser = $rows | Where-Object { [int]$_.island -eq $g } | Sort-Object { [int]$_.frame }
    $N = @($ser | ForEach-Object { [double]$_.population })
    if ($N.Count -lt 8) { continue }
    $pts = New-Object System.Collections.Generic.List[object]
    for ($t = 0; $t -lt $N.Count - 1; $t++) {
        $pts.Add([pscustomobject]@{ x = $N[$t]; y = $N[$t + 1] - $N[$t] })
        $pooled.Add($pts[$pts.Count - 1])
    }
    $cap = ($ser | Measure-Object -Property captures -Sum).Sum
    $esc = ($ser | Measure-Object -Property escapes -Sum).Sum
    $meanN = ($N | Measure-Object -Average).Average
    $sd = [Math]::Sqrt((($N | ForEach-Object { ($_ - $meanN) * ($_ - $meanN) } |
          Measure-Object -Sum).Sum) / $N.Count)
    $f = Fit $pts
    $nstar = "--"
    if ($f -and $f.slope -lt 0) { $nstar = [Math]::Round(-$f.inter / $f.slope, 2) }
    $tstat = 0.0; if ($f -and $f.se -gt 0) { $tstat = [Math]::Abs($f.slope / $f.se) }
    $perIsland += [pscustomobject]@{ g = $g; mean = [Math]::Round($meanN,2); sd =
        [Math]::Round($sd,2); min = ($N | Measure-Object -Minimum).Minimum;
        max = ($N | Measure-Object -Maximum).Maximum; slope =
        if ($f) { [Math]::Round($f.slope,4) } else { 0 }; t =
        [Math]::Round($tstat,1); nstar = $nstar; cap = [Math]::Round($cap/$N.Count,2) }
    $totalCap += $cap; $totalEsc += $esc; $totalN += $meanN; $totalSamples++
}

Write-Output "=============================================================="
Write-Output "ATTRACTOR ANALYSIS  ($Csv)"
Write-Output "frames=$frames  islands=$(($rows | Measure-Object -Property island -Minimum).Minimum)..$islands  samples/island=$($perIsland[0].g)`
"
$pf = Fit $pooled
Write-Output ("Pooled dN ~ N regression: {0} points" -f $pooled.Count)
if ($pf) {
    Write-Output ("  slope     = {0:E3}  (+- {1:E3})" -f $pf.slope, $pf.se)
    Write-Output ("  intercept = {0:E3}" -f $pf.inter)
    Write-Output ("  r2        = {0:F4}" -f $pf.r2)
    if ($pf.slope -lt 0) {
        Write-Output ("  ==> RESTORING DYNAMICS DETECTED;  N* ~= {0:F2}" -f (-$pf.inter / $pf.slope))
    } else {
        Write-Output "  ==> no restoring dynamics (slope >= 0)"
    }
}
Write-Output ("Mean affiliated population per island : {0:F2}" -f ($totalN / $totalSamples))
Write-Output ("Gross turnover per frame : captures={0:F1}  escapes={1:F1}" -f ($totalCap/$totalSamples), ($totalEsc/$totalSamples))
Write-Output ""
Write-Output "top islands by |t| of slope:"
$perIsland | Sort-Object { -$_.'t' } | Select-Object -First 10 |
    Format-Table @{l='island';e={$_.g}}, mean, sd, min, max, slope, t, nstar, cap -AutoSize | Out-String -Width 200
