$port = New-Object System.IO.Ports.SerialPort "COM8", 9600
$port.Open()
Start-Sleep -Milliseconds 1000
$port.WriteLine("1")
Start-Sleep -Milliseconds 4500
$out = $port.ReadExisting()
$port.Close()
Write-Host $out
