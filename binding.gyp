{
    'targets': [{
        'target_name': 'ssh'
      , 'conditions': [
            ['node_shared_openssl=="false"', {
                'include_dirs': [
                  '<(node_root_dir)/deps/openssl/openssl/include'
                ]
              , 'conditions' : [
                  ['target_arch=="ia32"', {
                    'include_dirs': [ '<(node_root_dir)/deps/openssl/config/piii' ]
                  }],
                  ['target_arch=="x64"', {
                    'include_dirs': [ '<(node_root_dir)/deps/openssl/config/k8' ]
                  }],
                  ['target_arch=="arm"', {
                    'include_dirs': [ '<(node_root_dir)/deps/openssl/config/arm' ]
                  }]
              ]
            }]
          , ['OS == "linux"', {
                'libraries': [
                    '-lcrypto'
                ]
            }]
          , ['OS == "solaris"', {
            }]
          , ['OS == "mac"', {
                'libraries': [
                    '-lssl'
                  , '-lcrypto'
                ]
            }]
          , ['OS == "win"', {
              'conditions': [
                # "openssl_root" is the directory on Windows of the OpenSSL files.
                # Check the "target_arch" variable to set good default values for
                # both 64-bit and 32-bit builds of the module.
                ['target_arch=="x64"', {
                  'variables': {
                    'openssl_root%': 'C:/OpenSSL-Win64'
                  },
                }, {
                  'variables': {
                    'openssl_root%': 'C:/OpenSSL-Win32'
                  },
                }],
              ],
              'defines': [
                    'LIBSSH_STATIC'
                ]
              ,
              'libraries': [
                '-l<(openssl_root)/lib/VC/static/libeay32MD.lib',
                '-l<(module_root_dir)/build/$(Configuration)/libssh.lib',
                'ws2_32.lib'
              ],
              'include_dirs': [
                '<(openssl_root)/include',
              ],
            }]
        ]
      , 'include_dirs' : [
            '<!(node -e "require(\'nan\')")'
        ]
      , 'dependencies': [
            '<(module_root_dir)/deps/libssh.gyp:libssh'
        ]
      , 'sources': [
            'src/nssh.cc'
          , 'src/server.cc'
          , 'src/session.cc'
          , 'src/channel.cc'
          , 'src/message.cc'
          , 'src/sftp_message.cc'
        ]
    }]
}
