// Copyright (c) 2013, Google Inc., James Golick
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
// Author: James Golick <jamesgolick@gmail.com>

#include <config.h>
#include "common.h"
#include "span.h"
#include <sys/resource.h>
#include "tlsf.h"

#include <stdio.h>

namespace tcmalloc {

tlsf_list_node_t* TLSF::NewNode(Span* value) {
  tlsf_list_node_t* result = _list_node_allocator->New();
  memset(result, 0, sizeof(*result));
  result->value = value;
  return result;
}

void TLSF::DeleteNode(tlsf_list_node_t* node) {
  _list_node_allocator->Delete(node);
}

void TLSF::Init(PageHeapAllocator<tlsf_list_node_t>* list_node_allocator) {
  _list_node_allocator = list_node_allocator;

  _fl_bitmap = 0;
  memset(&_sl_bitmaps, 0, sizeof(_sl_bitmaps));
  memset(&_blocks, 0, sizeof(_blocks));
}

void TLSF::Insert(Span* span) {
  size_t size = span->length << kPageShift;

  size_t fli = flindex(size);
  size_t sli = slindex(fli, size);

  tlsf_list_node_t *node = NewNode(span);

  if (_blocks[fli][sli]) {
    node->next = _blocks[fli][sli];
  }

  _blocks[fli][sli] = node;

  _fl_bitmap |= (1 << fli);
  _sl_bitmaps[fli] |= (1 << sli);
}

void TLSF::Remove(Span* span) {
  size_t size = span->length << kPageShift;

  size_t fli = flindex(size);
  size_t sli = slindex(fli, size);

  if (_blocks[fli][sli]) {
    tlsf_list_node_t* node = _blocks[fli][sli];

    if (node->value == span) {
      _blocks[fli][sli] = node->next;

      DeleteNode(node);
      clean_bitmaps(fli, sli);
    } else {
      do {
	if (node->next && node->next->value == span) {
	  tlsf_list_node_t* removed_node = node->next;
	  node->next = node->next->next;
	  DeleteNode(removed_node);
	  break;
	}
      } while((node = node->next) != NULL);
    }
  }
}

Span* TLSF::GetBestFit(size_t pages) {
  size_t size = pages << kPageShift;

  size_t fli = log2(_fl_bitmap & (~0 << flindex(size)));

  Span *rv = try_fli(fli, size);

  if (rv) {
    return rv;
  } else {
    return try_fli(fli + 1, size);
  }
}

inline Span* TLSF::try_fli(size_t fli, size_t size) {
  size_t sli = log2(_sl_bitmaps[fli] & (~0 << slindex(fli, size)));

  tlsf_list_node_t* node = _blocks[fli][sli];

  if (node) {
    Span* rv          = node->value;
    _blocks[fli][sli] = node->next;

    clean_bitmaps(fli, sli);

    DeleteNode(node);
    return rv;
  }

  return NULL;
}

void TLSF::clean_bitmaps(size_t fli, size_t sli) {
  if (_blocks[fli][sli] == NULL) {
    _sl_bitmaps[fli] &= ~(1 << sli);

    if (_sl_bitmaps[fli] == 0) {
      _fl_bitmap &= ~(1 << fli);
    }
  }
}

bool TLSF::Includes(Span* span) {
  return false;
}

size_t TLSF::log2(size_t size) {
  size_t i = 0;

  while (size >>= 1) { ++i; }

  return i;
}

size_t TLSF::flindex(size_t size) {
  return log2(size) - kPageShift;
}

size_t TLSF::slindex(size_t fli, size_t size) {
  size_t levels = (2 << (fli + kPageShift));

  if (levels > size) {
    return 0;
  } else {
    size_t levelw = (2 << (fli + kPageShift + 1)) - levels;
    return (size - levels) / (levelw / kSlLength);
  }
}

}
