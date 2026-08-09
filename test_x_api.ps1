param (
    [string]$BearerToken = ""
)

if ([string]::IsNullOrWhiteSpace($BearerToken)) {
    Write-Host "사용법: .\test_x_api.ps1 -BearerToken 'YOUR_BEARER_TOKEN'" -ForegroundColor Yellow
    exit
}

$url = "https://api.twitter.com/2/tweets/counts/recent?query=war%20OR%20conflict&granularity=minute"
$headers = @{
    "Authorization" = "Bearer $BearerToken"
}

Write-Host "X (Twitter) API v2 트윗 카운트 요청 중..." -ForegroundColor Cyan

try {
    $response = Invoke-RestMethod -Uri $url -Headers $headers -Method Get
    Write-Host "==========================================" -ForegroundColor Green
    Write-Host " 성공적으로 데이터를 수신했습니다!" -ForegroundColor Green
    Write-Host "==========================================" -ForegroundColor Green
    
    $count = $response.meta.total_tweet_count
    Write-Host "최근 1분간 트윗 수 (war OR conflict): $count 건" -ForegroundColor Gold
    Write-Host ""
    Write-Host "원문 응답 JSON:" -ForegroundColor Gray
    $response | ConvertTo-Json -Depth 5
}
catch {
    Write-Host "==========================================" -ForegroundColor Red
    Write-Host " API 호출 실패 (401 Unauthorized 등)" -ForegroundColor Red
    Write-Host "==========================================" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
}
