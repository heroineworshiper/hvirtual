autoreconf -f -i -I M4 thirdparty/libsndfile-*
autoreconf -f -i thirdparty/libiec61883-*
autoreconf -f -i quicktime/thirdparty/faac-*
autoreconf -f -i quicktime/thirdparty/faad2-*
./configure "$@"
