## GF-Complete
ExternalProject_Add (
    "gf-complete" 
    PREFIX "third-party/gf-complete"
    URL ${PROJECT_THIRD_PARTY_LIB_DIR}/gf-complete.zip
    URL_MD5 3293f8e0716ef91a136490ecba8dc592
    UPDATE_COMMAND ""
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./autogen.sh COMMAND ./configure ${THIRD_PARTY_CONFIG} --enable-shared=false --enable-static=true
    BUILD_COMMAND make
    INSTALL_DIR ""
    INSTALL_COMMAND make install
)

## ISA-L
ExternalProject_Add (
    "isa-l" 
    PREFIX "third-party/isa-l"
    URL ${PROJECT_THIRD_PARTY_LIB_DIR}/isa-l-2.31.1.tar.gz
    URL_MD5 851eb1b98da53c4dc8b94e0119106f03
    UPDATE_COMMAND ""
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./autogen.sh COMMAND ./configure ${THIRD_PARTY_CONFIG} --enable-shared=false --enable-static=true
    BUILD_COMMAND make
    INSTALL_DIR ""
    INSTALL_COMMAND make install
)