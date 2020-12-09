require 'rake/clean'

# File Extensions

SOURCE_EXTNAME = '.c'
HEADER_EXTNAME = '.h'
OBJECT_EXTNAME = '.o'
SHARED_OBJECT_EXTNAME = '.so'
PKG_EXTNAME = '.pc'

# Compiler Commands

CC = 'cc'
CFLAGS = '-Wall -Werror -fpic'

# Filenames/Paths

LIB = "libtheoraplay#{SHARED_OBJECT_EXTNAME}"
SOURCES = Rake::FileList["*#{SOURCE_EXTNAME}"]
OBJECTS = SOURCES.ext(OBJECT_EXTNAME)
HEADERS = Rake::FileList["*#{HEADER_EXTNAME}"]
PKGCONFIG = "theoraplay#{PKG_EXTNAME}"
PREFIX = '/usr/local'
LIB_DIR = "#{PREFIX}/lib"
INCLUDE_DIR = "#{PREFIX}/include/theoraplay"
PKGCONFIG_DIR = "#{LIB_DIR}/pkgconfig"

# Misc

VERBOSE = true

# ** TASKS **

desc 'Compiles sources into a dynamic library'
task build: OBJECTS do
    sh "#{CC} -shared -o #{LIB} #{OBJECTS} -logg -lvorbis -ltheora -ltheoradec -lpthread"
end

# As a general rule whenever we're dealing with source files, always convert them to object files first.
rule OBJECT_EXTNAME => SOURCE_EXTNAME do |task|
    sh "#{CC} #{CFLAGS} -c #{task.source}"
end

task :default => :build

CLEAN.include("*#{OBJECT_EXTNAME}")
CLOBBER.include("*#{SHARED_OBJECT_EXTNAME}")

desc "Copies the header, shared object, & pkgconfig file into #{PREFIX}"
task :install do
    FileUtils.mkdir_p(LIB_DIR, verbose: VERBOSE)
    FileUtils.mkdir_p(INCLUDE_DIR, verbose: VERBOSE)
    FileUtils.mkdir_p(PKGCONFIG_DIR, verbose: VERBOSE)

    HEADERS.each { |h| FileUtils.cp(h, INCLUDE_DIR, verbose: VERBOSE) }
    FileUtils.cp(LIB, LIB_DIR, verbose: VERBOSE)
    FileUtils.cp(PKGCONFIG, PKGCONFIG_DIR, verbose: VERBOSE)

    $stdout.puts("\nAdd export PKG_CONFIG_PATH=/foo/bar to your .bashrc, if your path isn't configured.")
    $stdout.puts("Add export LD_LIBRARY_PATH=/bar/foo to your .bashrc, to link shared object.")
end

desc "Removes the header, shared object, & pkgconfig file from #{PREFIX}"
task :uninstall do
    HEADERS.each { |h| FileUtils.rm("#{INCLUDE_DIR}/#{h}", verbose: VERBOSE) }
    FileUtils.rm("#{LIB_DIR}/#{LIB}", verbose: VERBOSE)
    FileUtils.rm("#{PKGCONFIG_DIR}/#{PKGCONFIG}", verbose: VERBOSE)
end
