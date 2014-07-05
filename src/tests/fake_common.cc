#include <stdlib.h> // for getenv and strtol
#include <stdint.h>
#include "internal_logging.h"

namespace tcmalloc {

void* MetaDataAlloc(size_t bytes) {
  return malloc(bytes);
}

void Log(LogMode, char const*, int, LogItem, LogItem, LogItem, LogItem) {
}

}
