default: ../TelnetOptions.h ../TelnetOptions.cpp ../TelnetCommands.h ../TelnetCommands.cpp

clean:
	rm ../TelnetOptions.h ../TelnetOptions.cpp ../TelnetCommands.h ../TelnetCommands.cpp

../TelnetOptions.h: ../telnet_options.txt
	./gen_TelnetOptions_h

../TelnetOptions.cpp: ../telnet_options.txt
	./gen_TelnetOptions_cpp

../TelnetCommands.cpp: ../telnet_commands.txt
	./gen_TelnetCommands_cpp

../TelnetCommands.h: ../telnet_commands.txt
	./gen_TelnetCommands_h
