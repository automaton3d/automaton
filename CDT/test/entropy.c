#include <stdio.h>
#include <string.h>
#include "md5.h"

/*
 * entropy.c
 *
 *  Created on: 1 de mai. de 2023
 *      Author: Alexandre
 */
unsigned char *entropy(char *str)
{
  static MD5_CTX context;
  MD5Init (&context);
  MD5Update (&context, str, strlen (str));
  MD5Final (&context);
  return context.digest;
}

