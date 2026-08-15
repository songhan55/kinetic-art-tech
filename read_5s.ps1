$port = New-Object System.IO.Ports.SerialPort "COM15", 115200
$port.Open()
Start-Sleep -Seconds 5
$out = $port.ReadExisting()
$port.Close()
Write-Host $out
