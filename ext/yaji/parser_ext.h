/*
 *  Author:: Couchbase <info@couchbase.com>
 *  Copyright:: 2011 Couchbase, Inc.
 *  License:: Apache License, Version 2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "api/yajl_parse.h"

#include <ruby.h>

#ifdef HAVE_RUBY_ENCODING_H
#include <ruby/encoding.h>
static rb_encoding *utf8_encoding;
#endif

/* Older versions of Ruby (< 1.8.6) need these */
#ifndef RSTRING_PTR
#define RSTRING_PTR(s) (RSTRING(s)->ptr)
#endif
#ifndef RSTRING_LEN
#define RSTRING_LEN(s) (RSTRING(s)->len)
#endif
#ifndef RARRAY_PTR
#define RARRAY_PTR(s) (RARRAY(s)->ptr)
#endif
#ifndef RARRAY_LEN
#define RARRAY_LEN(s) (RARRAY(s)->len)
#endif

static VALUE m_yaji, c_yaji_parser, c_parse_error, c_stringio;

static ID id_call, id_read, id_parse, id_perform, id_on_body, id_bytesize, id_strip;
static ID sym_allow_comments, sym_check_utf8, sym_symbolize_keys, sym_with_path,
	  sym_read_buffer_size, sym_null, sym_boolean, sym_number, sym_string,
	  sym_hash_key, sym_start_hash, sym_end_hash, sym_start_array,
	  sym_end_array;

static int yaji_null(void *ctx);
static int yaji_boolean(void *ctx, int val);
static int yaji_number(void *ctx, const char *val, unsigned int len);
static int yaji_string(void *ctx, const unsigned char *val, unsigned int len);
static int yaji_hash_key(void *ctx, const unsigned char *val, unsigned int len);
static int yaji_start_hash(void *ctx);
static int yaji_end_hash(void *ctx);
static int yaji_start_array(void *ctx);
static int yaji_end_array(void *ctx);

static yajl_callbacks yaji_callbacks = {
	yaji_null,
	yaji_boolean,
	NULL,
	NULL,
	yaji_number,
	yaji_string,
	yaji_start_hash,
	yaji_hash_key,
	yaji_end_hash,
	yaji_start_array,
	yaji_end_array
};

typedef struct {
	int symbolize_keys;
	int key_in_use;
	VALUE input;
	VALUE rbufsize;
	VALUE events;
	VALUE path;
	VALUE path_str;
	VALUE parser_cb;
	VALUE chunk;
	yajl_handle handle;
	yajl_parser_config config;
} yaji_parser;

static VALUE rb_yaji_parser_new(int argc, VALUE *argv, VALUE klass);
static VALUE rb_yaji_parser_init(int argc, VALUE *argv, VALUE self);
static VALUE rb_yaji_parser_parse(int argc, VALUE *argv, VALUE self);
static void rb_yaji_parser_free(void *parser);
static void rb_yaji_parser_mark(void *parser);

void Init_parser_ext();
