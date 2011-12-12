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

#include "parser_ext.h"

#define STATUS_CONTINUE 1

#define RB_P(OBJ) \
	rb_funcall(rb_stderr, rb_intern("print"), 1, rb_funcall(OBJ, rb_intern("object_id"), 0)); \
	rb_funcall(rb_stderr, rb_intern("print"), 1, rb_str_new2(" ")); \
	rb_funcall(rb_stderr, rb_intern("print"), 1, rb_funcall(OBJ, rb_intern("class"), 0)); \
	rb_funcall(rb_stderr, rb_intern("print"), 1, rb_str_new2(" ")); \
	rb_funcall(rb_stderr, rb_intern("puts"), 1, rb_funcall(OBJ, rb_intern("inspect"), 0));

#define RERAISE_PARSER_ERROR(parser) \
{ \
	unsigned char* emsg = yajl_get_error(parser->handle, 1, \
			(const unsigned char*)RSTRING_PTR(p->chunk), \
			RSTRING_LEN(p->chunk)); \
	VALUE errobj = rb_exc_new2(c_parse_error, (const char*) emsg); \
	yajl_free_error(parser->handle, emsg); \
	rb_exc_raise(errobj); \
}

static int yaji_null(void *ctx)
{
	yaji_parser* p = (yaji_parser*) DATA_PTR(ctx);
	VALUE rv = rb_ary_new3(3, p->path_str, sym_null, Qnil);
	rb_ary_push(p->events, rv);
	return STATUS_CONTINUE;
}

static int yaji_boolean(void *ctx, int val)
{
	yaji_parser* p = (yaji_parser*) DATA_PTR(ctx);
	VALUE rv = rb_ary_new3(3, p->path_str, sym_boolean, val ? Qtrue : Qfalse);
	rb_ary_push(p->events, rv);
	return STATUS_CONTINUE;
}

static int yaji_number(void *ctx, const char *val, unsigned int len)
{
	yaji_parser* p = (yaji_parser*) DATA_PTR(ctx);
	char buf[len+1];
	buf[len] = 0;
	memcpy(buf, val, len);
	VALUE rv;

	if (memchr(buf, '.', len) || memchr(buf, 'e', len) || memchr(buf, 'E', len)) {
		rv = rb_ary_new3(3, p->path_str, sym_number, rb_float_new(strtod(buf, NULL)));
	} else {
		rv = rb_ary_new3(3, p->path_str, sym_number, rb_cstr2inum(buf, 10));
	}
	rb_ary_push(p->events, rv);
	return STATUS_CONTINUE;
}

#ifdef HAVE_RUBY_ENCODING_H
#define YAJI_TO_STR(val, len, str)					    \
	str = rb_str_new((const char *)val, len);			    \
	rb_encoding *default_internal_enc = rb_default_internal_encoding(); \
	rb_enc_associate(str, utf8_encoding);				    \
	if (default_internal_enc) {					    \
		str = rb_str_export_to_enc(str, default_internal_enc);	    \
	}
#else
#define YAJI_TO_STR(val, len, str)					    \
	str = rb_str_new((const char *)val, len);
#endif

static int yaji_string(void *ctx, const unsigned char *val, unsigned int len)
{
	yaji_parser* p = (yaji_parser*) DATA_PTR(ctx);
	VALUE str, rv;
	YAJI_TO_STR((const char *)val, len, str);
	rv = rb_ary_new3(3, p->path_str, sym_string, str);
	rb_ary_push(p->events, rv);
	return STATUS_CONTINUE;
}

static int yaji_hash_key(void *ctx, const unsigned char *val, unsigned int len)
{
	yaji_parser* p = (yaji_parser*) DATA_PTR(ctx);
	VALUE key, rv;
	YAJI_TO_STR((const char *)val, len, key);
	key = p->symbolize_keys ? ID2SYM(rb_to_id(key)) : key;
	if (p->key_in_use) {
		rb_ary_pop(p->path);
	} else {
		p->key_in_use = 1;
	}
	p->path_str = rb_ary_join(p->path, rb_str_new2("/"));
	rb_str_freeze(p->path_str);
	rv = rb_ary_new3(3, p->path_str, sym_hash_key, key);
	rb_ary_push(p->events, rv);
	rb_ary_push(p->path, key);
	p->path_str = rb_ary_join(p->path, rb_str_new2("/"));
	rb_str_freeze(p->path_str);
	return STATUS_CONTINUE;
}

static int yaji_start_hash(void *ctx)
{
	yaji_parser* p = (yaji_parser*) DATA_PTR(ctx);
	p->key_in_use = 0;
	VALUE rv = rb_ary_new3(3, p->path_str, sym_start_hash, Qnil);
	rb_ary_push(p->events, rv);
	return STATUS_CONTINUE;
}

static int yaji_end_hash(void *ctx)
{
	yaji_parser* p = (yaji_parser*) DATA_PTR(ctx);
	rb_ary_pop(p->path);
	p->path_str = rb_ary_join(p->path, rb_str_new2("/"));
	VALUE rv = rb_ary_new3(3, p->path_str, sym_end_hash, Qnil);
	rb_ary_push(p->events, rv);
	return STATUS_CONTINUE;
}

static int yaji_start_array(void *ctx)
{
	yaji_parser* p = (yaji_parser*) DATA_PTR(ctx);
	VALUE rv = rb_ary_new3(3, p->path_str, sym_start_array, Qnil);
	rb_ary_push(p->path, rb_str_new2(""));
	p->path_str = rb_ary_join(p->path, rb_str_new2("/"));
	rb_str_freeze(p->path_str);
	rb_ary_push(p->events, rv);
	return STATUS_CONTINUE;
}

static int yaji_end_array(void *ctx)
{
	yaji_parser* p = (yaji_parser*) DATA_PTR(ctx);

	rb_ary_pop(p->path);
	p->path_str = rb_ary_join(p->path, rb_str_new2("/"));

	VALUE rv = rb_ary_new3(3, p->path_str, sym_end_array, Qnil);
	rb_ary_push(p->events, rv);
	return STATUS_CONTINUE;
}

static VALUE rb_yaji_parser_parse_chunk(VALUE chunk, VALUE self)
{
	yajl_status rc;
	yaji_parser* p = (yaji_parser*) DATA_PTR(self);
	const char* buf;
	unsigned int len;
	int i;

	if (NIL_P(chunk) || (len = RSTRING_LEN(chunk)) == 0) {
		return INT2FIX(0);
	}
	buf = RSTRING_PTR(chunk);
	p->events = rb_ary_new();
	p->chunk = chunk;
	rc = yajl_parse(p->handle, (const unsigned char*)buf, len);
	if (rc == yajl_status_error) {
		RERAISE_PARSER_ERROR(p);
	}
	for (i=0; i<RARRAY_LEN(p->events); i++) {
		rb_funcall(p->parser_cb, id_call, 1, RARRAY_PTR(p->events)[i]);
	}
	return rb_funcall(chunk, id_bytesize, 0, NULL);
}

static VALUE rb_yaji_parser_new(int argc, VALUE *argv, VALUE klass)
{
	yaji_parser* p;
	VALUE opts, obj;

	obj = Data_Make_Struct(klass, yaji_parser, rb_yaji_parser_mark, rb_yaji_parser_free, p);
	p->handle = NULL;
	p->config.allowComments = 1;
	p->config.checkUTF8 = 1;
	p->symbolize_keys = 0;
	p->rbufsize = Qnil;
	p->input = Qnil;
	p->parser_cb = Qnil;

	rb_scan_args(argc, argv, "11", &p->input, &opts);
	if (TYPE(p->input) == T_STRING) {
		p->input = rb_class_new_instance(1, &p->input, c_stringio);
	} else if (rb_respond_to(p->input, id_perform) && rb_respond_to(p->input, id_on_body)) {
		rb_block_call(p->input, id_on_body, 0, NULL, rb_yaji_parser_parse_chunk, obj);
	} else if (!rb_respond_to(p->input, id_read)) {
		rb_raise(c_parse_error, "input must be a String or IO or "
				"something responding to #perform and #on_body e.g. Curl::Easy");
	}
	if (!NIL_P(opts)) {
		Check_Type(opts, T_HASH);
		if (rb_hash_aref(opts, sym_allow_comments) == Qfalse) {
			p->config.allowComments = 0;
		}
		if (rb_hash_aref(opts, sym_check_utf8) == Qfalse) {
			p->config.checkUTF8 = 0;
		}
		if (rb_hash_aref(opts, sym_symbolize_keys) == Qtrue) {
			p->symbolize_keys = 1;
		}
		p->rbufsize = rb_hash_aref(opts, sym_read_buffer_size);
	}
	if (NIL_P(p->rbufsize)) {
		p->rbufsize = INT2FIX(READ_BUFSIZE);
	} else {
		Check_Type(p->rbufsize, T_FIXNUM);
	}
	p->handle = yajl_alloc(&yaji_callbacks, &p->config, NULL, (void *)obj);
	rb_obj_call_init(obj, 0, 0);
	return obj;
}

static VALUE rb_yaji_parser_init(int argc, VALUE *argv, VALUE self)
{
	return self;
}


static VALUE rb_yaji_parser_parse(int argc, VALUE* argv, VALUE self)
{
	yajl_status rc;
	yaji_parser* p = (yaji_parser*) DATA_PTR(self);
	int i;

	rb_scan_args(argc, argv, "00&", &p->parser_cb);
	RETURN_ENUMERATOR(self, argc, argv);

	p->path = rb_ary_new();
	p->path_str = rb_str_new("", 0);
	p->chunk = Qnil;

	if (rb_respond_to(p->input, id_perform)) {
		rb_funcall(p->input, id_perform, 0);
	} else {
		p->chunk = rb_str_new(NULL, 0);
		while (rb_funcall(p->input, id_read, 2, p->rbufsize, p->chunk) != Qnil) {
			rb_yaji_parser_parse_chunk(p->chunk, self);
		}
	}

	p->events = rb_ary_new();
	rc = yajl_parse_complete(p->handle);

	if (rc == yajl_status_error ||
			(rc == yajl_status_insufficient_data && RSTRING_LEN(rb_funcall(p->chunk, id_strip, 0)) != 0)) {
		RERAISE_PARSER_ERROR(p);
	}
	for (i=0; i<RARRAY_LEN(p->events); i++) {
		rb_funcall(p->parser_cb, id_call, 1, RARRAY_PTR(p->events)[i]);
	}

	return Qnil;
}

static int rb_yaji_str_start_with(VALUE str, VALUE query)
{
	int i;
	const char *ptr = RSTRING_PTR(str);
	int len = RSTRING_LEN(str);
	VALUE entry;

	switch(TYPE(query)) {
	case T_STRING:
		return RSTRING_LEN(query) <= len && memcmp(RSTRING_PTR(query), ptr, RSTRING_LEN(query)) == 0;
		break;
	case T_ARRAY:
		for (i=0; i<RARRAY_LEN(query); i++) {
			entry = RARRAY_PTR(query)[i];
			if (RSTRING_LEN(entry) <= len && memcmp(RSTRING_PTR(entry), ptr, RSTRING_LEN(entry)) == 0) {
				return 1;
			}
		}
		break;
	}
	return 0;
}

static VALUE rb_yaji_each_iter(VALUE chunk, VALUE* params_p)
{
	VALUE* params = (VALUE*)params_p;
	VALUE path = rb_ary_shift(chunk);
	VALUE event = rb_ary_shift(chunk);
	VALUE value = rb_ary_shift(chunk);
	VALUE proc = params[0];
	VALUE stack = params[1];
	VALUE query = params[2];
	VALUE with_path = params[3];
	VALUE last_entry, object, container, key, hash;

	if (NIL_P(query) || rb_yaji_str_start_with(path, query)) {
		if (event == sym_hash_key) {
			rb_ary_push(stack, value);
		} else if (event == sym_start_hash || event == sym_start_array) {
			container = (event == sym_start_hash) ? rb_hash_new() : rb_ary_new();
			last_entry = rb_ary_entry(stack, -1);
			switch(TYPE(last_entry)) {
			case T_STRING:
				key = rb_ary_pop(stack);
				hash = rb_ary_entry(stack, -1);
				rb_hash_aset(hash, key, container);
				break;
			case T_ARRAY:
				rb_ary_push(last_entry, container);
			}
			rb_ary_push(stack, container);
		} else if (event == sym_end_hash || event == sym_end_array) {
			object = rb_ary_pop(stack);
			if (RARRAY_LEN(stack) == 0) {
				if (with_path == Qnil || with_path == Qfalse) {
					rb_funcall(proc, id_call, 1, object);
				} else {
					rb_funcall(proc, id_call, 1, rb_ary_new3(2, path, object));
				}
			}
		} else {
			last_entry = rb_ary_entry(stack, -1);
			switch(TYPE(last_entry)) {
			case T_STRING:
				key = rb_ary_pop(stack);
				hash = rb_ary_entry(stack, -1);
				rb_hash_aset(hash, key, value);
				break;
			case T_ARRAY:
				rb_ary_push(last_entry, value);
				break;
			case T_NIL:
				if (with_path == Qnil || with_path == Qfalse) {
					rb_funcall(proc, id_call, 1, value);
				} else {
					rb_funcall(proc, id_call, 1, rb_ary_new3(2, path, value));
				}
				break;
			}
		}
	}
	return Qnil;
}

static VALUE rb_yaji_parser_each(int argc, VALUE* argv, VALUE self)
{
	VALUE query, proc, options, params[4];
	RETURN_ENUMERATOR(self, argc, argv);
	rb_scan_args(argc, argv, "02&", &query, &options, &proc);
	params[0] = proc;	    // callback
	params[1] = rb_ary_new();   // stack
	params[2] = query;
	if (options != Qnil) {
		Check_Type(options, T_HASH);
		params[3] = rb_hash_aref(options, sym_with_path);
	} else {
		params[3] = Qnil;
	}
	rb_block_call(self, id_parse, 0, NULL, rb_yaji_each_iter, (VALUE)params);
	return Qnil;
}

static void rb_yaji_parser_free(void *parser)
{
	yaji_parser* p = parser;
	if (p) {
		if (p->handle) {
			yajl_free(p->handle);
		}
		free(p);
	}
}

static void rb_yaji_parser_mark(void *parser)
{
	yaji_parser* p = parser;
	if (p) {
		rb_gc_mark(p->input);
		rb_gc_mark(p->rbufsize);
		rb_gc_mark(p->events);
		rb_gc_mark(p->path);
		rb_gc_mark(p->path_str);
		rb_gc_mark(p->parser_cb);
		rb_gc_mark(p->chunk);
	}
}

/* Ruby Extension initializer */
void Init_parser_ext() {
	m_yaji = rb_define_module("YAJI");

	c_parse_error = rb_define_class_under(m_yaji, "ParseError", rb_eStandardError);

	c_yaji_parser = rb_define_class_under(m_yaji, "Parser", rb_cObject);
	rb_define_const(c_yaji_parser, "READ_BUFFER_SIZE", INT2FIX(READ_BUFSIZE));
	rb_define_singleton_method(c_yaji_parser, "new", rb_yaji_parser_new, -1);
	rb_define_method(c_yaji_parser, "initialize", rb_yaji_parser_init, -1);
	rb_define_method(c_yaji_parser, "parse", rb_yaji_parser_parse, -1);
	rb_define_method(c_yaji_parser, "each", rb_yaji_parser_each, -1);

	id_call = rb_intern("call");
	id_read = rb_intern("read");
	id_parse = rb_intern("parse");
	id_strip = rb_intern("strip");
	id_perform = rb_intern("perform");
	id_on_body = rb_intern("on_body");
	id_bytesize = rb_intern("bytesize");

	sym_allow_comments = ID2SYM(rb_intern("allow_comments"));
	sym_check_utf8 = ID2SYM(rb_intern("check_utf8"));
	sym_symbolize_keys = ID2SYM(rb_intern("symbolize_keys"));
	sym_read_buffer_size = ID2SYM(rb_intern("read_buffer_size"));
	sym_with_path = ID2SYM(rb_intern("with_path"));
	sym_null = ID2SYM(rb_intern("null"));
	sym_boolean = ID2SYM(rb_intern("boolean"));
	sym_number = ID2SYM(rb_intern("number"));
	sym_string = ID2SYM(rb_intern("string"));
	sym_hash_key = ID2SYM(rb_intern("hash_key"));
	sym_start_hash = ID2SYM(rb_intern("start_hash"));
	sym_end_hash = ID2SYM(rb_intern("end_hash"));
	sym_start_array = ID2SYM(rb_intern("start_array"));
	sym_end_array = ID2SYM(rb_intern("end_array"));

#ifdef HAVE_RUBY_ENCODING_H
	utf8_encoding = rb_utf8_encoding();
#endif

	rb_require("stringio");
	c_stringio = rb_const_get(rb_cObject, rb_intern("StringIO"));
}
