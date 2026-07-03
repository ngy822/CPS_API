@echo off
:: 设置只对当前窗口生效的代理变量
set HTTP_PROXY=http://127.0.0.1:7897
set HTTPS_PROXY=http://127.0.0.1:7897
set NODE_TLS_REJECT_UNAUTHORIZED=0

start "" "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe"

exit