build_example(
  NAME myfirst
  SOURCE_FILES myfirst.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libpoint-to-point}
    ${libinternet}
    ${libapplications}
    ${libnetwork}
)

build_example(
  NAME abm
  SOURCE_FILES abm.cc cdf.cc
  HEADER_FILES cdf.h
  LIBRARIES_TO_LINK
    ${libcore}
    ${libpoint-to-point}
    ${libinternet}
    ${libapplications}
    ${libnetwork}
    ${libtraffic-control}
)
