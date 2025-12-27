/* Copyright (c) 2025-2026. The FSMod Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "fsmod/version.hpp"

void fsmod_version_get(int* ver_major, int* ver_minor, int* ver_patch)
{
  *ver_major = FSMOD_VERSION_MAJOR;
  *ver_minor = FSMOD_VERSION_MINOR;
  *ver_patch = FSMOD_VERSION_PATCH;
}
