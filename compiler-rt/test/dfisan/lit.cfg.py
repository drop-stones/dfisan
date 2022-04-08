# -*- Python -*-

import os

# Setup config name.
config.name = 'DataFlowIntegritySanitizer' + config.name_suffix

# Setup source root.
config.test_source_root = os.path.dirname(__file__)

# Setup default compiler flags used with -fsanitize=dfi option.
clang_dfisan_cflags = (["-fsanitize=dfi", "-O0"] + [config.target_cflags])
clang_dfisan_cxxflags = config.cxx_mode_flags + clang_dfisan_cflags

def build_invocation(compile_flags):
  return " " + " ".join([config.clang] + compile_flags) + " "

config.substitutions.append( ("%clang_dfisan ", build_invocation(clang_dfisan_cflags)) )
config.substitutions.append( ("%clangxx_dfisan ", build_invocation(clang_dfisan_cxxflags)) )

# Default test suffixes.
config.suffixes = ['.c', '.cpp']

# DataFlowIntegritySanitizer tests are currently supported on Linux only.
if config.host_os not in ['Linux']:
  config.unsupported = True