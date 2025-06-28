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