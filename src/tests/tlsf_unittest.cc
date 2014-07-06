// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*-
// Copyright (c) 2005, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ---
// Author: James Golick

#include <stddef.h>                     // for size_t, NULL
#include <stdlib.h>                     // for malloc, free, realloc
#include <stdio.h>
#include <set>                          // for set, etc

#include "base/logging.h"               // for operator<<, CHECK, etc

#include "tlsf.h"

using std::set;

int main (int argc, char** argv) {
  tcmalloc::Span span;
  span.length = 10;

  tcmalloc::Span span2;
  span2.length = 20;

  tcmalloc::PageHeapAllocator<tcmalloc::tlsf_list_node_t> alloc;
  alloc.Init();

  tcmalloc::TLSF tlsf;

  tlsf.Init(&alloc);

  span.length = 129;
  tlsf.Insert(&span);

  CHECK_EQ(tlsf.GetBestFit(130), NULL);

  tlsf.Remove(&span);

  span.length = 10;
  tlsf.Insert(&span);
  tlsf.Insert(&span2);

  CHECK_EQ(tlsf.GetBestFit(11), &span2);
  CHECK_EQ(tlsf.GetBestFit(4), &span);
  CHECK_EQ(tlsf.GetBestFit(4), NULL);
  CHECK_EQ(tlsf.GetBestFit(20), NULL);

  tlsf.Insert(&span);
  CHECK_EQ(tlsf.GetBestFit(1), &span);
  CHECK_EQ(tlsf.GetBestFit(1), NULL);

  span.length  = 5;
  span2.length = 5;

  tlsf.Insert(&span);
  tlsf.Insert(&span2);

  CHECK_EQ(tlsf.GetBestFit(10), NULL);
  CHECK_EQ(tlsf.GetBestFit(5), &span2);
  CHECK_EQ(tlsf.GetBestFit(5), &span);
  CHECK_EQ(tlsf.GetBestFit(5), NULL);

  // Check the basic case of remove, where the 
  // node is the first and only node in the list.
  tlsf.Insert(&span);
  tlsf.Remove(&span);

  CHECK_EQ(tlsf.GetBestFit(1), NULL);


  // Check the more complex version where the node
  // is deeper in the list.
  tlsf.Insert(&span);
  tlsf.Insert(&span2);
  tlsf.Remove(&span);
  tlsf.Remove(&span2);

  CHECK_EQ(tlsf.GetBestFit(1), NULL);

  printf("PASS\n");
  return 0;
}
