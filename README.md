# Theoraplay
A fork of Ryan C. Gordan's Theoraplay. No major changes, just more documentation, and actual build system(so now building is easier), & a pkg-config file so including this in your project becomes effortless.
Also the tests here are more detailed and up-to-date, thanks to https://www.glusoft.com/tuto/play-video-SDL2.php. This was tested on Linux, & Windows.

# Dependencies
```
Ruby, pkg-config, SDL2, libogg, libvorbis, & libtheora.
```

## Run Tests
```ruby
cd tests
rake -T # To view test tasks
```

## Install
```ruby
rake -T # To view the list of tasks.

rake # To build
rake install # To install
rake uninstall # To uninstall
```

## Change Install Path
By default the installation path is /usr/local, but this can change by editing the Rakefile, & the theoraplay.pc file.
```ruby
# In Rakefile
PREFIX = "#{Dir.home}/some/dir"
```
```shell
# In theoraplay.pc
prefix=home/some/dir
```
