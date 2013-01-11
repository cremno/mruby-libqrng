# Prerequisites
- register an account on https://qrng.physik.hu-berlin.de
- download libQRNG from https://qrng.physik.hu-berlin.de/download
- MinGW users: "ar rcs libQRNG.a libQRNG.lib" to convert the import library

# Installation
- use bovi/mgem or add the gem by yourself to build_config.rb

# Usage
```ruby
qrng = QRNG.new 'AzureDiamond', 'hunter2', :ssl  # username, password, use ssl (optional)
pw = qrng.password 13, 'ABCdef123'  # password_length, tobeused_password_chars (optional)
puts pw  # => 2dBfCC123CfeC
int = qrng.data :int  # byte (String), int (Fixnum), double (Float (between 0 and 1))
puts int  # => 455379
int = qrng.data :int, 3
puts int  # => [-984391892, 1743334411, -1048288211]
qrng.disconnect
```

# License
MIT License: http://opensource.org/licenses/MIT
