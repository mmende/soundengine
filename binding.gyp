{
	"targets": [
		{
			"target_name": "soundengine",
			"sources": [
				"src/SoundEngine.cpp",
				"src/WindowFunction.cpp"
			],
			"include_dirs": [
				"<!(node -e \"require('nan')\")",
				"<(module_root_dir)/src"
			],
			'conditions' : [
				[
					'OS=="mac"', {
						"xcode_settings": {
							"OTHER_CPLUSPLUSFLAGS" : [ "-std=c++11", "-stdlib=libc++" ],
							"OTHER_LDFLAGS": [ "-stdlib=libc++" ],
							"MACOSX_DEPLOYMENT_TARGET": "10.7"
						},
						'include_dirs': [
							'<!@(pkg-config portaudio-2.0 --cflags-only-I)',
							'<!@(pkg-config fftw3 --cflags-only-I)'
						],
						"libraries": [
							'<!@(pkg-config portaudio-2.0 --libs-only-l)',
							'<!@(pkg-config fftw3 --libs)',
							'/Library/Frameworks/AudioToolbox.framework',
							'/Library/Frameworks/AudioUnit.framework',
							'/Library/Frameworks/Carbon.framework'
						],
					}
				],
				[
					'OS=="linux"', {
						'include_dirs': [
							'<!@(pkg-config portaudio-2.0 --cflags-only-I)',
							'<!@(pkg-config fftw3 --cflags-only-I)'
						],
						'libraries' : [
							'<!@(pkg-config portaudio-2.0 --libs)',
							'<!@(pkg-config fftw3 --libs)'
						],
						'cflags!': [ '-fno-exceptions' ],
						'cflags_cc!': [ '-fno-exceptions' ],
						'cflags_cc': [ '-std=c++0x' ]
					}
				],
				[
					'OS=="win"', {}
				]
			],
			"cflags": [
				"-std=c++11"
			],
			"cflags_cc!": [ '-fno-rtti' ]
		}
	]
}