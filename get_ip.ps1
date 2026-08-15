$port = New-Object System.IO.Ports.SerialPort "COM15", 115200
$port.DtrEnable = $false
$port.RtsEnable = $true
$port.Open()
Start-Sleep -Milliseconds 100
$port.RtsEnable = $false
Start-Sleep -Seconds 4
$out = $port.ReadExisting()
$port.Close()
Write-Host $out
