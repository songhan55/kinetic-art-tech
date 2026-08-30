@echo off
chcp 65001 > nul
title Kinetic Art Real-Time Control Center
start "" "http://localhost:8000"
python C:\art_tech\kinetic_realtime_system\run_dashboard_bridge.py
pause
