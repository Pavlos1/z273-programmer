A very simple EPROM programmer intended for use with
[Z273 modules](https://www.radiomuseum.org/r/midlandint_eprom_module_z_273.html).
Similar to [this](https://dbindner.freeshell.org/z273/) but without any of the
safety features. Requires an Arduino Mega 2560 and a regulated 21V/25V voltage
source.

Supports:
* Programming in-situ (i.e. connecting directly to a Z273 module)
* Programming a stand-alone EPROM

EPROMs supported:
* 2716 (UVEPROM)
* AT28C16 (EEPROM)

All code is released to the public domain.
