/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*
 * This compatible header includes corresponding header files for getopt_xxx functions
 * depending on toolchain/c_runtime: For GNU/newlib, the getopt.h are included by default;
 * For IAR/Dlib, the ported iar_getopt are used.
 */

#if defined(__IAR_SYSTEMS_ICC__)
  #include "iar_getopt.h"
#elif defined(__NUTTX__)
  #include "nuttx_getopt.h"
#else
  #include "getopt.h"
#endif
