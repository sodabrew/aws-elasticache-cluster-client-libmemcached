/*  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 * 
 *  Test memexist
 *
 *  Copyright (C) 2011 Data Differential, http://datadifferential.com/
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


/*
  Test that we are cycling the servers we are creating during testing.
*/

#include <config.h>

#include <libtest/test.hpp>
#include <libmemcached/memcached.h>
#include <libmemcached/util.h>

using namespace libtest;

#ifndef __INTEL_COMPILER
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

static std::string executable("./clients/memexist");

static test_return_t help_test(void *)
{
  const char *args[]= { "--help", 0 };

  test_compare(EXIT_SUCCESS, exec_cmdline(executable, args, true));
  return TEST_SUCCESS;
}

static test_return_t exist_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--server=localhost:%d", int(default_port()));
  const char *args[]= { buffer, "foo", 0 };

  memcached_st *memc= memcached(buffer, strlen(buffer));
  test_true(memc);

  test_compare(MEMCACHED_SUCCESS,
               memcached_set(memc, test_literal_param("foo"), 0, 0, 0, 0));

  memcached_return_t rc;
  test_null(memcached_get(memc, test_literal_param("foo"), 0, 0, &rc));
  test_compare(MEMCACHED_SUCCESS, rc);

  test_compare(EXIT_SUCCESS, exec_cmdline(executable, args, true));

  test_null(memcached_get(memc, test_literal_param("foo"), 0, 0, &rc));
  test_compare(MEMCACHED_SUCCESS, rc);

  memcached_free(memc);

  return TEST_SUCCESS;
}

static test_return_t NOT_FOUND_test(void *)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--server=localhost:%d", int(default_port()));
  const char *args[]= { buffer, "foo", 0 };

  memcached_st *memc= memcached(buffer, strlen(buffer));
  test_true(memc);

  test_compare(MEMCACHED_SUCCESS, memcached_flush(memc, 0));

  memcached_return_t rc;
  test_null(memcached_get(memc, test_literal_param("foo"), 0, 0, &rc));
  test_compare(MEMCACHED_NOTFOUND, rc);

  test_compare(EXIT_FAILURE, exec_cmdline(executable, args, true));

  test_null(memcached_get(memc, test_literal_param("foo"), 0, 0, &rc));
  test_compare(MEMCACHED_NOTFOUND, rc);

  memcached_free(memc);

  return TEST_SUCCESS;
}

static test_return_t check_version(void*)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "--server=localhost:%d", int(default_port()));
  memcached_st *memc= memcached(buffer, strlen(buffer));
  test_true(memc);
  
  test_return_t result= TEST_SUCCESS;
  if (libmemcached_util_version_check(memc, 1, 4, 8) == false)
  {
    result= TEST_SKIPPED;
  }
  memcached_free(memc);

  return result;
}

test_st memexist_tests[] ={
  {"--help", true, help_test },
  {"exist(FOUND)", true, exist_test },
  {"exist(NOT_FOUND)", true, NOT_FOUND_test },
  {0, 0, 0}
};

collection_st collection[] ={
  {"memexist", check_version, 0, memexist_tests },
  {0, 0, 0, 0}
};

static void *world_create(server_startup_st& servers, test_return_t& error)
{
  if (HAVE_MEMCACHED_BINARY == 0)
  {
    error= TEST_SKIPPED;
    return NULL;
  }

  if (server_startup(servers, "memcached", libtest::default_port(), 0, NULL) == false)
  {
    error= TEST_FAILURE;
  }

  return &servers;
}


void get_world(Framework *world)
{
  world->collections(collection);
  world->create(world_create);
}

