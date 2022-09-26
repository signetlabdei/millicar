set(source_files
    model/mmwave-vehicular.cc
    model/mmwave-sidelink-spectrum-phy.cc
    model/mmwave-sidelink-spectrum-signal-parameters.cc
    model/mmwave-sidelink-phy.cc
    model/mmwave-sidelink-mac.cc
    model/mmwave-vehicular-net-device.cc
    model/mmwave-vehicular-antenna-array-model.cc
    helper/mmwave-vehicular-helper.cc
    helper/mmwave-vehicular-traces-helper.cc
)

set(test_sources
    test/mmwave-vehicular-spectrum-phy-test.cc
    test/mmwave-vehicular-rate-test.cc
    test/mmwave-vehicular-interference-test.cc
)

set(header_files
    model/mmwave-vehicular.h
    model/mmwave-sidelink-spectrum-phy.h
    model/mmwave-sidelink-spectrum-signal-parameters.h
    model/mmwave-sidelink-phy.h
    model/mmwave-sidelink-mac.h
    model/mmwave-sidelink-sap.h
    model/mmwave-vehicular-net-device.h
    model/mmwave-vehicular-antenna-array-model.h
    helper/mmwave-vehicular-helper.h
    helper/mmwave-vehicular-traces-helper.h
)

build_lib(
  LIBNAME millicar
  SOURCE_FILES ${source_files}
  HEADER_FILES ${header_files}
  LIBRARIES_TO_LINK
    ${libcore}
    ${libpropagation}
    ${libspectrum}
    ${libmmwave}
  TEST_SOURCES ${test_sources}
)