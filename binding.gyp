{
    "targets": [
        {
            "target_name": "windump",
            "includes": [
                "auto.gypi"
            ],
            "conditions": [
                ['OS=="win"', {
                    "sources": [
                        "src_win/dumper.cpp"
                    ],
                }]
            ],
            "include_dirs": [
              "<!(node -p \"require('node-addon-api').include_dir\")",
            ],
            "dependencies": [
              "<!(node -p \"require('node-addon-api').gyp\")"
            ],
            "libraries": [
            ],
            "cflags!": ["-fno-exceptions"],
            "cflags_cc!": ["-fno-exceptions"],
            "defines": [
                "UNICODE",
                "_UNICODE"
            ],
            "msvs_settings": {
                "VCCLCompilerTool": {
                    "ExceptionHandling": 1
                }
            }
        }
    ],
    "includes": [
        "auto-top.gypi"
    ]
}
