logman create trace -n HID_WPP -o %SystemRoot%\Tracing\HID_WPP.etl -nb 128 640 -bs 128
logman update trace -n HID_WPP -p {47c779cd-4efd-49d7-9b10-9f16e5c25d06} 0x7FFFFFFF 0xFF
logman update trace -n HID_WPP -p {896f2806-9d0e-4d5f-aa25-7acdbf4eaf2c} 0x7FFFFFFF 0xFF
logman start -n HID_WPP
