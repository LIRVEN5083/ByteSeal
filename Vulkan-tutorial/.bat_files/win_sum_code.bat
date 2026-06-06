@echo off

echo Code stroke:

powershell -Command "((& '.\cloc-2.08.exe' --exclude-dir=external '%~dp0..\src' | Select-String 'SUM:') -split '\s+' | Where-Object { $_ -match '^\d+$' } | Measure-Object -Sum).Sum"
\


pause