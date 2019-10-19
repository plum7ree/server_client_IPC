- Compile libyaml:

  Required packages:
  - gcc
  - libtool
  - make

  cd libyaml/
  ./bootstrap
  ./configure
  make
  cd [project_root]
  cp libyaml/src/.libs/libyaml.so code
  cp libyaml/src/.libs/libyaml-0.so.2 code
  cp libyaml/include/yaml.h code/

- Compile TinyFile:
  cd code/
  make clean
  make

- Run the server and client (change the args as needed)
  LD_LIBRARY_PATH=./ ./server --n_sms 1 --sms_size 64
  LD_LIBRARY_PATH=./ ./client --conf ../input/test.yaml --state ASYNC


- Run the stress test and draw graph. This will output csv to ouput/ folder and run the python to draw png graph files.
  (matplotlib required)
  ./tester.sh
