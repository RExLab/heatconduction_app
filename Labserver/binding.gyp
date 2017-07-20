{
  "targets": [
    {
      "target_name": "panel",
	  "sources": [ 
		  "src/panel.cc", 
		  "src/crc/crc.c",
		  "src/uart/uart.c",
		  "src/timer/timer.c",
		  "src/modbus/modbus_master.c",
		  "src/modbus/modbus_slave.c",
		  "src/timer/timer.h",
		  "src/modbus/modbus.h",
		  "src/modbus/modbus_master.h",
		  "src/modbus/modbus_slave.h",
		  "src/uart/uart.h",
		  "src/app.h",
		  "src/_config_lib_.h", 
		  "src/_config_cpu_.h", 
		  "src/raspberry.h",
		  "src/arch.h",
		  "src/crc/crc.h", 
		  "src/uc_libdefs.h"		  
		  ],
		  
      "include_dirs": [
		  "<!(node -e \"require('nan')\")",
		"src", "src/uart", "src/modbus","src/crc","src/timer"
		  
      ]
    }
  ]
}
