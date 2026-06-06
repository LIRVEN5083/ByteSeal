@echo off

echo Code stroke:

powershell -Command "((& 'C:\Users\lirve\OneDrive\Desktop\cloc-2.08.exe' 'C:\Users\lirve\OneDrive\Desktop\ByteSeal\Vulkan-tutorial' | Select-String 'SUM:') -split '\s+' | Where-Object { $_ -match '^\d+$' } | Measure-Object -Sum).Sum"

pause