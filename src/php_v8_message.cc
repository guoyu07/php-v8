/*
 * This file is part of the pinepain/php-v8 PHP extension.
 *
 * Copyright (c) 2015-2017 Bogdan Padalko <pinepain@gmail.com>
 *
 * Licensed under the MIT license: http://opensource.org/licenses/MIT
 *
 * For the full copyright and license information, please view the
 * LICENSE file that was distributed with this source or visit
 * http://opensource.org/licenses/MIT
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_v8_message.h"
#include "php_v8_stack_trace.h"
#include "php_v8_script_origin.h"
#include "php_v8_value.h"
#include "php_v8.h"

zend_class_entry* php_v8_message_class_entry;
#define this_ce php_v8_message_class_entry

void php_v8_message_create_from_message(zval *return_value, php_v8_isolate_t *php_v8_isolate, v8::Local<v8::Message> message) {

    assert(!message.IsEmpty());

    object_init_ex(return_value, this_ce);

    v8::Isolate *isolate = php_v8_isolate->isolate;
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    /* v8::Message::Get */
    if (!message->Get().IsEmpty()) {
        v8::String::Utf8Value message_utf8(message->Get());
        PHP_V8_CONVERT_UTF8VALUE_TO_STRING_WITH_CHECK(message_utf8, message_chars);
        zend_update_property_string(this_ce, return_value, ZEND_STRL("message"), message_chars);
    }

    /* v8::Message::GetSourceLine */
    if (!message->GetSourceLine(context).IsEmpty()) {
        v8::String::Utf8Value source_line_utf8(message->GetSourceLine(context).ToLocalChecked());
        PHP_V8_CONVERT_UTF8VALUE_TO_STRING_WITH_CHECK(source_line_utf8, source_line_chars);
        zend_update_property_string(this_ce, return_value, ZEND_STRL("source_line"), source_line_chars);
    }

    /* v8::Message::GetScriptOrigin */
    zval origin_zv;
    php_v8_create_script_origin(&origin_zv, context, message->GetScriptOrigin());
    zend_update_property(this_ce, return_value, ZEND_STRL("script_origin"), &origin_zv);
    zval_ptr_dtor(&origin_zv);

    /* v8::Message::GetScriptResourceName */
    if (!message->GetScriptResourceName().IsEmpty() && !message->GetScriptResourceName()->IsUndefined()) {
        v8::String::Utf8Value script_resource_name_utf8(message->GetScriptResourceName());
        PHP_V8_CONVERT_UTF8VALUE_TO_STRING_WITH_CHECK(script_resource_name_utf8, script_resource_name_chars);
        zend_update_property_string(this_ce, return_value, ZEND_STRL("resource_name"), script_resource_name_chars);
    }

    /* v8::Message::GetStackTrace */
    if (!message->GetStackTrace().IsEmpty()) {
        zval trace_zv;
        php_v8_stack_trace_create_from_stack_trace(&trace_zv, php_v8_isolate, message->GetStackTrace());
        zend_update_property(this_ce, return_value, ZEND_STRL("stack_trace"), &trace_zv);
        zval_ptr_dtor(&trace_zv);
    }

    /* v8::Message::GetLineNumber */
    /* NOTE: we don't use FromMaybe(v8::Message::kNoLineNumberInfo) due to static const (https://gcc.gnu.org/wiki/VerboseDiagnostics#missing_static_const_definition)*/
    int line_number = v8::Message::kNoLineNumberInfo;
    if (!message->GetLineNumber(context).IsNothing()) {
        line_number = message->GetLineNumber(context).FromJust();
    }
    zend_update_property_long(this_ce, return_value, ZEND_STRL("line_number"), static_cast<zend_long>(line_number));

    /* v8::Message::GetStartPosition */
    zend_update_property_long(this_ce, return_value, ZEND_STRL("start_position"), static_cast<zend_long>(message->GetStartPosition()));

    /* v8::Message::GetEndPosition */
    zend_update_property_long(this_ce, return_value, ZEND_STRL("end_position"), static_cast<zend_long>(message->GetEndPosition()));

    /* v8::Message::GetStartColumn */
    /* NOTE: we don't use FromMaybe(v8::Message::kNoColumnInfo) due to static const (https://gcc.gnu.org/wiki/VerboseDiagnostics#missing_static_const_definition)*/
    int start_column = v8::Message::kNoColumnInfo;
    if (!message->GetStartColumn(context).IsNothing()) {
        start_column = message->GetStartColumn(context).FromJust();
    }

    zend_update_property_long(this_ce, return_value, ZEND_STRL("start_column"), static_cast<zend_long>(start_column));

    /* v8::Message::GetEndColumn */
    /* NOTE: we don't use FromMaybe(v8::Message::kNoColumnInfo) due to static const (https://gcc.gnu.org/wiki/VerboseDiagnostics#missing_static_const_definition)*/
    int end_column  = v8::Message::kNoColumnInfo;
    if (!message->GetEndColumn(context).IsNothing()) {
        end_column  = message->GetEndColumn(context).FromJust();
    }
    zend_update_property_long(this_ce, return_value, ZEND_STRL("end_column"), static_cast<zend_long>(end_column));

    /* v8::Message::IsSharedCrossOrigin */
    zend_update_property_bool(this_ce, return_value, ZEND_STRL("is_shared_cross_origin"), static_cast<zend_bool>(message->IsSharedCrossOrigin()));
    /* v8::Message::IsOpaque */
    zend_update_property_bool(this_ce, return_value, ZEND_STRL("is_opaque"), static_cast<zend_bool>(message->IsOpaque()));
}


static PHP_METHOD(V8Message, __construct) {

    zend_string *message = NULL;
    zend_string *source_line = NULL;
    zval *script_origin = NULL;
    zend_string *resource_name = NULL;
    zval *stack_trace = NULL;

    zend_long line_number = static_cast<zend_long>(v8::Message::kNoLineNumberInfo);
    zend_long start_position = -1;
    zend_long end_position = -1;
    zend_long start_column = static_cast<zend_long>(v8::Message::kNoColumnInfo);
    zend_long end_column = static_cast<zend_long>(v8::Message::kNoColumnInfo);

    zend_bool is_shared_cross_origin = '\0';
    zend_bool is_opaque = '\0';

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "SSoSo|lllllbb",
                              &message, &source_line, &script_origin, &resource_name, &stack_trace,
                              &line_number, &start_position, &end_position, &start_column, &end_column,
                              &is_shared_cross_origin, &is_opaque) == FAILURE) {
        return;
    }

    zend_update_property_str(this_ce, getThis(), ZEND_STRL("message"), message);
    zend_update_property_str(this_ce, getThis(), ZEND_STRL("source_line"), source_line);
    zend_update_property(this_ce, getThis(), ZEND_STRL("script_origin"), script_origin);
    zend_update_property_str(this_ce, getThis(), ZEND_STRL("resource_name"), resource_name);
    zend_update_property(this_ce, getThis(), ZEND_STRL("stack_trace"), stack_trace);

    zend_update_property_long(this_ce, getThis(), ZEND_STRL("line_number"), line_number);
    zend_update_property_long(this_ce, getThis(), ZEND_STRL("start_position"), start_position);
    zend_update_property_long(this_ce, getThis(), ZEND_STRL("end_position"), end_position);
    zend_update_property_long(this_ce, getThis(), ZEND_STRL("start_column"), start_column);
    zend_update_property_long(this_ce, getThis(), ZEND_STRL("end_column"), end_column);

    zend_update_property_bool(this_ce, getThis(), ZEND_STRL("is_shared_cross_origin"), is_shared_cross_origin);
    zend_update_property_bool(this_ce, getThis(), ZEND_STRL("is_opaque"), is_opaque);
}

static PHP_METHOD(V8Message, Get)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("message"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Message, GetSourceLine)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("source_line"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Message, GetScriptOrigin)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("script_origin"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Message, GetScriptResourceName)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("resource_name"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Message, GetStackTrace)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("stack_trace"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Message, GetLineNumber)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("line_number"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Message, GetStartPosition)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("start_position"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Message, GetEndPosition)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("end_position"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Message, GetStartColumn)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("start_column"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Message, GetEndColumn)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("end_column"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Message, IsSharedCrossOrigin)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("is_shared_cross_origin"), 0, &rv), 1, 0);
}

static PHP_METHOD(V8Message, IsOpaque)
{
    zval rv;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    RETVAL_ZVAL(zend_read_property(this_ce, getThis(), ZEND_STRL("is_opaque"), 0, &rv), 1, 0);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_v8_message___construct, ZEND_SEND_BY_VAL, ZEND_RETURN_VALUE, 5)
                ZEND_ARG_TYPE_INFO(0, message, IS_STRING, 0)
                ZEND_ARG_TYPE_INFO(0, source_line, IS_STRING, 0)
                ZEND_ARG_OBJ_INFO(0, script_origin, V8\\ScriptOrigin, 0)
                ZEND_ARG_TYPE_INFO(0, resource_name, IS_STRING, 0)
                ZEND_ARG_OBJ_INFO(0, stack_trace, V8\\StackTrace, 0)
                ZEND_ARG_TYPE_INFO(0, line_number, IS_LONG, 0)
                ZEND_ARG_TYPE_INFO(0, start_position, IS_LONG, 0)
                ZEND_ARG_TYPE_INFO(0, end_position, IS_LONG, 0)
                ZEND_ARG_TYPE_INFO(0, start_column, IS_LONG, 0)
                ZEND_ARG_TYPE_INFO(0, end_column, IS_LONG, 0)
                ZEND_ARG_TYPE_INFO(0, is_shared_cross_origin, _IS_BOOL, 0)
                ZEND_ARG_TYPE_INFO(0, is_opaque, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_message_Get, ZEND_RETURN_VALUE, 0, IS_STRING, NULL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_message_GetSourceLine, ZEND_RETURN_VALUE, 0, IS_STRING, NULL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_message_GetScriptOrigin, ZEND_RETURN_VALUE, 0, IS_OBJECT, PHP_V8_NS "\\ScriptOrigin", 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_message_GetScriptResourceName, ZEND_RETURN_VALUE, 0, IS_STRING, NULL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_message_GetStackTrace, ZEND_RETURN_VALUE, 0, IS_OBJECT, PHP_V8_NS "\\StackTrace", 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_message_GetLineNumber, ZEND_RETURN_VALUE, 0, IS_LONG, NULL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_message_GetStartPosition, ZEND_RETURN_VALUE, 0, IS_LONG, NULL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_message_GetEndPosition, ZEND_RETURN_VALUE, 0, IS_LONG, NULL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_message_GetStartColumn, ZEND_RETURN_VALUE, 0, IS_LONG, NULL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_message_GetEndColumn, ZEND_RETURN_VALUE, 0, IS_LONG, NULL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_message_IsSharedCrossOrigin, ZEND_RETURN_VALUE, 0, _IS_BOOL, NULL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_v8_message_IsOpaque, ZEND_RETURN_VALUE, 0, _IS_BOOL, NULL, 0)
ZEND_END_ARG_INFO()


static const zend_function_entry php_v8_message_methods[] = {
        PHP_ME(V8Message, __construct, arginfo_v8_message___construct, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)

        PHP_ME(V8Message, Get, arginfo_v8_message_Get, ZEND_ACC_PUBLIC)
        PHP_ME(V8Message, GetSourceLine, arginfo_v8_message_GetSourceLine, ZEND_ACC_PUBLIC)
        PHP_ME(V8Message, GetScriptOrigin, arginfo_v8_message_GetScriptOrigin, ZEND_ACC_PUBLIC)
        PHP_ME(V8Message, GetScriptResourceName, arginfo_v8_message_GetScriptResourceName, ZEND_ACC_PUBLIC)

        PHP_ME(V8Message, GetStackTrace, arginfo_v8_message_GetStackTrace, ZEND_ACC_PUBLIC)

        PHP_ME(V8Message, GetLineNumber, arginfo_v8_message_GetLineNumber, ZEND_ACC_PUBLIC)
        PHP_ME(V8Message, GetStartPosition, arginfo_v8_message_GetStartPosition, ZEND_ACC_PUBLIC)
        PHP_ME(V8Message, GetEndPosition, arginfo_v8_message_GetEndPosition, ZEND_ACC_PUBLIC)

        PHP_ME(V8Message, GetStartColumn, arginfo_v8_message_GetStartColumn, ZEND_ACC_PUBLIC)
        PHP_ME(V8Message, GetEndColumn, arginfo_v8_message_GetEndColumn, ZEND_ACC_PUBLIC)

        PHP_ME(V8Message, IsSharedCrossOrigin, arginfo_v8_message_IsSharedCrossOrigin, ZEND_ACC_PUBLIC)
        PHP_ME(V8Message, IsOpaque, arginfo_v8_message_IsOpaque, ZEND_ACC_PUBLIC)

        PHP_FE_END
};

PHP_MINIT_FUNCTION (php_v8_message) {
    zend_class_entry ce;
    INIT_NS_CLASS_ENTRY(ce, PHP_V8_NS, "Message", php_v8_message_methods);
    this_ce = zend_register_internal_class(&ce);

    zend_declare_class_constant_long(this_ce, ZEND_STRL("kNoLineNumberInfo"), static_cast<zend_long>(v8::Message::kNoLineNumberInfo));
    zend_declare_class_constant_long(this_ce, ZEND_STRL("kNoColumnInfo"), static_cast<zend_long>(v8::Message::kNoColumnInfo));
    zend_declare_class_constant_long(this_ce, ZEND_STRL("kNoScriptIdInfo"), static_cast<zend_long>(v8::Message::kNoLineNumberInfo));

    zend_declare_property_string(this_ce, ZEND_STRL("message"), "", ZEND_ACC_PRIVATE);
    zend_declare_property_null(this_ce, ZEND_STRL("script_origin"), ZEND_ACC_PRIVATE);
    zend_declare_property_string(this_ce, ZEND_STRL("source_line"), "", ZEND_ACC_PRIVATE);
    zend_declare_property_string(this_ce, ZEND_STRL("resource_name"), "", ZEND_ACC_PRIVATE);
    zend_declare_property_null(this_ce, ZEND_STRL("stack_trace"), ZEND_ACC_PRIVATE);

    zend_declare_property_long(this_ce, ZEND_STRL("line_number"), static_cast<zend_long>(v8::Message::kNoLineNumberInfo), ZEND_ACC_PRIVATE);
    zend_declare_property_long(this_ce, ZEND_STRL("start_position"), -1, ZEND_ACC_PRIVATE);
    zend_declare_property_long(this_ce, ZEND_STRL("end_position"), -1, ZEND_ACC_PRIVATE);

    zend_declare_property_long(this_ce, ZEND_STRL("start_column"), static_cast<zend_long>(v8::Message::kNoColumnInfo), ZEND_ACC_PRIVATE);
    zend_declare_property_long(this_ce, ZEND_STRL("end_column"), static_cast<zend_long>(v8::Message::kNoColumnInfo), ZEND_ACC_PRIVATE);

    zend_declare_property_bool(this_ce, ZEND_STRL("is_shared_cross_origin"), static_cast<zend_bool>(false), ZEND_ACC_PRIVATE);
    zend_declare_property_bool(this_ce, ZEND_STRL("is_opaque"), static_cast<zend_bool>(false), ZEND_ACC_PRIVATE);

    return SUCCESS;
}
