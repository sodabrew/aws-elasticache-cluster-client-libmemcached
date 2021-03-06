/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Libmemcached library
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
 *  Copyright (C) 2010 Brian Aker All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *      * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *  copyright notice, this list of conditions and the following disclaimer
 *  in the documentation and/or other materials provided with the
 *  distribution.
 *
 *      * The names of its contributors may not be used to endorse or
 *  promote products derived from this software without specific prior
 *  written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <libmemcached/common.h>

const char * memcached_lib_version(void) 
{
  return LIBMEMCACHED_VERSION_STRING;
}

static inline memcached_return_t memcached_version_textual(memcached_st *ptr)
{
  libmemcached_io_vector_st vector[]=
  {
    { memcached_literal_param("version\r\n") },
  };

  uint32_t success= 0;
  bool errors_happened= false;
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++)
  {
    memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, x);

    // Optimization, we only fetch version once.
    if (instance->major_version != UINT8_MAX)
    {
      continue;
    }

    memcached_return_t rrc;
    if (memcached_failed(rrc= memcached_vdo(instance, vector, 1, true)))
    {
      errors_happened= true;
      (void)memcached_set_error(*instance, rrc, MEMCACHED_AT);
      continue;
    }
    success++;
  }

  if (success)
  {
    // Collect the returned items
    memcached_server_write_instance_st instance;
    while ((instance= memcached_io_get_readable_server(ptr)))
    {
      memcached_return_t rrc= memcached_response(instance, NULL);
      if (memcached_failed(rrc))
      {
        memcached_io_reset(instance);
        errors_happened= true;
      }
    }
  }

  return errors_happened ? MEMCACHED_SOME_ERRORS : MEMCACHED_SUCCESS;
}

static inline memcached_return_t memcached_version_binary(memcached_st *ptr)
{
  protocol_binary_request_version request= {};
  request.message.header.request.magic= PROTOCOL_BINARY_REQ;
  request.message.header.request.opcode= PROTOCOL_BINARY_CMD_VERSION;
  request.message.header.request.datatype= PROTOCOL_BINARY_RAW_BYTES;

  libmemcached_io_vector_st vector[]=
  {
    { request.bytes, sizeof(request.bytes) }
  };

  uint32_t success= 0;
  bool errors_happened= false;
  for (uint32_t x= 0; x < memcached_server_count(ptr); x++) 
  {
    memcached_server_write_instance_st instance= memcached_server_instance_fetch(ptr, x);

    if (instance->major_version != UINT8_MAX)
    {
      continue;
    }

    memcached_return_t rrc= memcached_vdo(instance, vector, 1, true);
    if (memcached_failed(rrc))
    {
      memcached_io_reset(instance);
      errors_happened= true;
      continue;
    }

    success++;
  }

  if (success)
  {
    // Collect the returned items
    memcached_server_write_instance_st instance;
    while ((instance= memcached_io_get_readable_server(ptr)))
    {
      char buffer[32];
      memcached_return_t rrc= memcached_response(instance, buffer, sizeof(buffer), NULL);
      if (memcached_failed(rrc))
      {
        memcached_io_reset(instance);
        errors_happened= true;
      }
    }
  }

  return errors_happened ? MEMCACHED_SOME_ERRORS : MEMCACHED_SUCCESS;
}

memcached_return_t memcached_version(memcached_st *ptr)
{
  memcached_return_t rc;
  if (memcached_failed(rc= initialize_query(ptr, true)))
  {
    return rc;
  }

  if (memcached_is_udp(ptr))
  {
    return MEMCACHED_NOT_SUPPORTED;
  }

  if (memcached_is_binary(ptr))
  {
    return memcached_version_binary(ptr);
  }

  return memcached_version_textual(ptr);      
}
