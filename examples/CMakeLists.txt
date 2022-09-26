set(base_examples
    vehicular-simple-one
    vehicular-simple-two
    vehicular-simple-three
    vehicular-simple-four
    mmwave-vehicular-link-adaptation-example
)

foreach(
  example
  ${base_examples}
)
  build_lib_example(
    NAME ${example}
    SOURCE_FILES ${example}.cc
    LIBRARIES_TO_LINK 
        ${libmillicar}
        ${libcore}
        ${libpropagation}
        ${libspectrum}        
        ${libmmwave}
  )
endforeach()