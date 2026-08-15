$port = New-Object System.IO.Ports.SerialPort "COM15", 115200
$port.ReadTimeout = 3000
$port.Open()
Start-Sleep -Milliseconds 3000
$out = $port.ReadExisting()
$port.Close()
Write-Host $out
