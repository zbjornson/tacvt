{
  "targets": [
    {
      "target_name": "tacvt",
      "sources": [
        "init.cc"
      ],
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
      ],
      "cflags": [
        "-std=c++14",
        "-march=native",
        "-Wno-cast-function-type"
      ],
      "cflags_cc!": [
        "-std=gnu++0x" # Allow -std=c++14 to prevail
      ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "EnableEnhancedInstructionSet": 5 # /arch:AVX2
        }
      },
      "xcode_settings": {
        "GCC_VERSION": "com.apple.compilers.llvm.clang.1_0",
        "CLANG_CXX_LANGUAGE_STANDARD": "c++14",
        "CLANG_CXX_LIBRARY": "libc++",
        "OTHER_CPLUSPLUSFLAGS": [
          "-march=native"
        ]
      }
    }
  ]
}
