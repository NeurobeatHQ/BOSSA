# ESP-IDF component registration. Lets this repo be vendored into an
# ESP-IDF project (e.g. under components/bossa or via EXTRA_COMPONENT_DIRS).
# Ignored by the Arduino build, which only compiles src/ per the library spec.
#
# The core is plain C++ with no IDF headers: file I/O goes through
# newlib/VFS (mount a filesystem before Flasher::write) and the consumer
# provides a SerialPort implementation (e.g. over the UART driver).
# No CONFIG_COMPILER_CXX_EXCEPTIONS needed.
idf_component_register(
    SRCS
        "src/Applet.cpp"
        "src/Device.cpp"
        "src/Flash.cpp"
        "src/Flasher.cpp"
        "src/NullFlash.cpp"
        "src/Samba.cpp"
        "src/WordCopyApplet.cpp"
        "src/WordCopyArm.cpp"
    INCLUDE_DIRS "src")

# Upstream format strings use %x/%X for uint32_t, which newlib on xtensa
# types as long unsigned int; benign (same width), so keep the sources
# untouched and silence -Wformat locally instead.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format)
