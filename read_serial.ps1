$port = New-Object System.IO.Ports.SerialPort "COM15", 115200, "None", 8, "One"
$port.ReadTimeout = 1000
try {
    $port.Open()
    $endTime = (Get-Date).AddSeconds(3)
    while ((Get-Date) -lt $endTime) {
        if ($port.BytesToRead -gt 0) {
            $msg = $port.ReadExisting()
            Write-Host $msg -NoNewline
        }
        Start-Sleep -Milliseconds 50
    }
} catch {
    Write-Host "Serial Error: $_"
} finally {
    if ($port.IsOpen) {
        $port.Close()
    }
}
