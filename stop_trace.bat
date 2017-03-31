logman stop -n HID_WPP
logman delete -n HID_WPP
move /Y %SystemRoot%\Tracing\*.etl .\
