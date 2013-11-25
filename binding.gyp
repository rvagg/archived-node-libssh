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
