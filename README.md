mruby-qrng
==========

# Prerequisites
- register an account on https://qrng.physik.hu-berlin.de
- download libQRNG from https://qrng.physik.hu-berlin.de/download
- MinGW users: "ar rcs libQRNG.a libQRNG.lib" to convert the import library

# Usage
```ruby
q = QRNG.new 'AzureDiamond', 'hunter2'  # username, password
pw = q.generate_password 'ABCdef123', 13  # tobeused_password_chars, password_length
q.disconnect
puts pw  # => 2dBfCC123CfeC

# alternative
QRNG.new('AzureDiamond', 'hunter2') do |q|
  pw = q.generate_password 'ABCdef123', 13
  puts pw
end
```

# License
MIT License: http://opensource.org/licenses/MIT
