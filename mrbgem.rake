MRuby::Gem::Specification.new('mruby-libqrng') do |spec|
  spec.license = 'MIT'
  spec.authors = 'cremno'
  spec.mruby.linker.libraries << 'libQRNG'
end
