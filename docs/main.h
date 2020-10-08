/**
 * \file
 * Main page of the Doxygen-generated documentation.
 */
/**
 * \mainpage MilliCar Documentation
 *
 * \section intro-sec Introduction
 * <a href="http://www.nsnam.org/">ns-3</a> documentation is maintained using 
 * <a href="http://www.doxygen.org">Doxygen</a>.
 * Doxygen is typically used for 
 * API documentation, and organizes such documentation across different
 * modules.   This project uses Doxygen for building the definitive
 * maintained API documentation.  Additional ns-3 project documentation 
 * can be found at the
 * <a href="http://www.nsnam.org/documentation/latest">project web site</a>.
 *
 * \section install-sec Building the Documentation
 * 
 * ns-3 requires Doxygen version 1.8.3.1 or greater.
 * 
 * Type "./waf --doxygen" or "./waf --doxygen-no-build" to build the 
 *  documentation.  The doc/ directory contains
 * configuration for Doxygen (doxygen.conf) and main.h.  The Doxygen 
 * build process puts html files into the doc/html/ directory, and latex 
 * filex into the doc/latex/ directory.
 * 
 */
/**
 * \namespace ns3
 * \brief Every class exported by the ns3 library is enclosed in the
 * ns3 namespace.
 */


/**
 * \name Macros defined by the build system.
 * 
 * These have to be visible for doxygen to document them,
 * so we put them here in a file only seen by doxygen, not the compiler.
 * 
 * @{
 */
/**
 * \ingroup assert
 *
 * \def NS3_ASSERT_ENABLE
 *
 * Enable asserts at compile time.
 *
 * This is normally set by `./waf configure --build-profile=debug`.
 */
#define NS3_ASSERT_ENABLE

/**
 * \ingroup logging
 *
 * \def NS3_LOG_ENABLE
 *
 * Enable logging at compile time.
 *
 * This is normally set by `./waf configure --build-profile=debug`.
 */
#define NS3_LOG_ENABLE

/**@}*/