/*
   Copyright (C) 2010 Anton Mihalyov <anton@bytepaper.com>

   This  library is  free software;  you can  redistribute it  and/or
   modify  it under  the  terms  of the  GNU  Library General  Public
   License  (LGPL)  as published  by  the  Free Software  Foundation;
   either version  2 of the  License, or  (at your option)  any later
   version.

   This library  is distributed in the  hope that it will  be useful,
   but WITHOUT  ANY WARRANTY;  without even  the implied  warranty of
   MERCHANTABILITY or FITNESS  FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy  of the GNU Library General Public
   License along with this library; see the file COPYING.LIB. If not,
   write to the  Free Software Foundation, Inc.,  51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <errno.h>
#include <limits.h>
#include <iconv.h>
#include <string.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

#include <eb/eb.h>
#include <eb/error.h>
#include <eb/text.h>

#include <v8.h>

#include "config.hh"
#include "error.hh"
#include "utils.hh"

#include "epwing-dictionary-js.hh"
#include "epwing-dictionary.hh"

namespace simplify {


static EB_Error_Code HandleIso8859_1(EB_Book *, EB_Appendix *, void *,
                                     EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleJisX0208(EB_Book *, EB_Appendix *, void *,
                                    EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleGb2312(EB_Book *, EB_Appendix *, void *,
                                  EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginSub(EB_Book *, EB_Appendix *, void *,
                                    EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndSub(EB_Book *, EB_Appendix *, void *,
                                  EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginSup(EB_Book *, EB_Appendix *, void *,
                                    EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndSup(EB_Book *, EB_Appendix *, void *,
                                  EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleIndent(EB_Book *, EB_Appendix *, void *,
                                  EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleNewline(EB_Book *, EB_Appendix *, void *,
                                   EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginNoBr(EB_Book *, EB_Appendix *, void *,
                                     EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndNoBr(EB_Book *, EB_Appendix *, void *,
                                   EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginEm(EB_Book *, EB_Appendix *, void *,
                                   EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndEm(EB_Book *, EB_Appendix *, void *,
                                 EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginReference(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndReference(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginKeyword(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndKeyword(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleBeginDecoration(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleEndDecoration(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleInsertHGaiji(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);
static EB_Error_Code HandleInsertTGaiji(EB_Book *, EB_Appendix *, void *,
                                      EB_Hook_Code, int, const unsigned int *);


static const EB_Hook g_head_hooks[] = {
    { EB_HOOK_ISO8859_1, &HandleIso8859_1 },
    { EB_HOOK_NARROW_JISX0208, &HandleJisX0208 },
    { EB_HOOK_WIDE_JISX0208, &HandleJisX0208 },
    { EB_HOOK_GB2312, &HandleGb2312 },
    { EB_HOOK_NARROW_FONT, &HandleInsertHGaiji },
    { EB_HOOK_WIDE_FONT, &HandleInsertHGaiji },
    { EB_HOOK_NULL, NULL }
};

static const EB_Hook g_text_hooks[] = {
    { EB_HOOK_ISO8859_1, &HandleIso8859_1 },
    { EB_HOOK_NARROW_JISX0208, &HandleJisX0208 },
    { EB_HOOK_WIDE_JISX0208, &HandleJisX0208 },
    { EB_HOOK_GB2312, &HandleGb2312 },
    { EB_HOOK_NARROW_FONT, &HandleInsertTGaiji },
    { EB_HOOK_WIDE_FONT, &HandleInsertTGaiji },
    { EB_HOOK_BEGIN_SUBSCRIPT, &HandleBeginSub },
    { EB_HOOK_END_SUBSCRIPT, &HandleEndSub },
    { EB_HOOK_BEGIN_SUPERSCRIPT, &HandleBeginSup },
    { EB_HOOK_END_SUPERSCRIPT, &HandleEndSup },
    { EB_HOOK_SET_INDENT, &HandleIndent },
    { EB_HOOK_NEWLINE, &HandleNewline },
    { EB_HOOK_BEGIN_NO_NEWLINE, &HandleBeginNoBr },
    { EB_HOOK_END_NO_NEWLINE, &HandleEndNoBr },
    { EB_HOOK_BEGIN_EMPHASIS, &HandleBeginEm },
    { EB_HOOK_END_EMPHASIS, &HandleEndEm },
    { EB_HOOK_BEGIN_REFERENCE, &HandleBeginReference },
    { EB_HOOK_END_REFERENCE, &HandleEndReference },
    { EB_HOOK_BEGIN_KEYWORD, &HandleBeginKeyword },
    { EB_HOOK_END_KEYWORD, &HandleEndKeyword },
    { EB_HOOK_BEGIN_DECORATION, &HandleBeginDecoration },
    { EB_HOOK_END_DECORATION, &HandleEndDecoration },
    { EB_HOOK_NULL, NULL }
};

enum class JsFunction : size_t {
    BeginSubscript      = 0,
    EndSubscript        = 1,
    BeginSuperscript    = 2,
    EndSuperscript      = 3,
    Indent              = 4,
    Newline             = 5,
    BeginNoBreak        = 6,
    EndNoBreak          = 7,
    BeginEmphasis       = 8,
    EndEmphasis         = 9,
    BeginReference      = 10,
    EndReference        = 11,
    BeginKeyword        = 12,
    EndKeyword          = 13,
    BeginDecoration     = 14,
    EndDecoration       = 15,
    InsertTextGaiji     = 16,
    ProcessHeading      = 17,
    ProcessTags         = 18,
    ProcessText         = 19,
    InsertHeadingGaiji  = 20
};

static const char *g_js_function_names[] = {
    /* JsFunction:00 */ "BeginSubscript",
    /* JsFunction:01 */ "EndSubscript",
    /* JsFunction:02 */ "BeginSuperscript",
    /* JsFunction:03 */ "EndSuperscript",
    /* JsFunction:04 */ "Indent",
    /* JsFunction:05 */ "Newline",
    /* JsFunction:06 */ "BeginNoBreak",
    /* JsFunction:07 */ "EndNoBreak",
    /* JsFunction:08 */ "BeginEmphasis",
    /* JsFunction:09 */ "EndEmphasis",
    /* JsFunction:10 */ "BeginReference",
    /* JsFunction:11 */ "EndReference",
    /* JsFunction:12 */ "BeginKeyword",
    /* JsFunction:13 */ "EndKeyword",
    /* JsFunction:14 */ "BeginDecoration",
    /* JsFunction:15 */ "EndDecoration",
    /* JsFunction:16 */ "InsertTextGaiji",
    /* JsFunction:17 */ "ProcessHeading",
    /* JsFunction:18 */ "ProcessTags",
    /* JsFunction:19 */ "ProcessText",
    /* JsFunction:20 */ "InsertHeadingGaiji"
};

static const size_t g_js_function_count = \
    sizeof(g_js_function_names) / sizeof(g_js_function_names[0]);

/**
 * Initializes libeb. The function calls library initialization routine only
 * once. Subsequent calls to this function will do nothing.
 */
static bool InitializeLibEb(std::error_code &error)
{
    static bool init = false;

    if (!init) {
        EB_Error_Code eb_code = eb_initialize_library();
        if (eb_code == EB_SUCCESS) {
            init = true;
            return true;
        } else {
            error = make_error_code(static_cast<eb_error>(eb_code));
            return false;
        }
    } else {
        return true;
    }
}

static v8::Persistent<v8::Script> &GetDefaultJsScript()
{
    using namespace v8;

    static Persistent<Script> script =
        Persistent<Script>::New(
                Script::New(String::New(g_default_js_implementation)));

    return script;
}

/**
 * Encodes source string specified in the variable @input using iconv conversion
 * descriptor @cd.
 *
 * \param cd iconv conversion descriptor.
 * \param input A pointer to the source string to encode.
 * \param input_size Size of the @input.
 * \param buffer A pointer to the buffer where resulting string should be
 *  stored.
 * \param buffer_size Size of the buffer pointed by @buffer.
 * \param error Reference to std::error_code where an error, if any, will be
 *  stored. If encoding succeeds the error state will be cleared.
 *
 * \return If succeeds, returns number of bytes in the @buffer string. If fails,
 *  <em>(size_t) -1</em> will be returned.
 */
static size_t Iconv(iconv_t cd,
                    const char *input,
                    size_t input_size,
                    char *buffer,
                    size_t buffer_size,
                    std::error_code &error)
{
    error.clear();

    if (cd == (iconv_t) -1) {
        error = make_error_code(static_cast<std::errc>(errno));
        return -1;
    }

    size_t buffer_left = buffer_size;
    size_t input_left = input_size;

    // XXX: I have no idea why glibc does require a char ** pointer to
    // the inbuf while other implementations require only const char ** ...
    // According to the man page, inbuf will not be touched, so it should
    // be safe to strip const.
    char *input_tmp = const_cast<char *>(input);

    size_t result =
        iconv(cd, &input_tmp, &input_left, &buffer, &buffer_left);

    if (result != (size_t) -1) {
        return buffer_size - buffer_left;
    } else {
        // Translate E2BIG error code into similar simplify error to
        // allow special handling of this case.
        if (errno == E2BIG)
            error = make_error_code(simplify_error::buffer_exhausted);
        else
            error = make_error_code(static_cast<std::errc>(errno));

        return -1;
    }
}

/**
 * Encodes source UTF-8 string @input to EUC-JP string and stores it into
 * @buffer with the max size of @buffer_size.
 */
inline static size_t ConvertUtf8ToEucJp(const char *input,
                                        size_t input_size,
                                        char *buffer,
                                        size_t buffer_size,
                                        std::error_code &error)
{
    static iconv_t cd = iconv_open("EUC-JP", "UTF-8");
    return Iconv(cd, input, input_size, buffer, buffer_size, error);
}

inline static size_t ConvertUtf8ToIso8859_1(const char *input,
                                            size_t input_size,
                                            char *buffer,
                                            size_t buffer_size,
                                            std::error_code &error)
{
    static iconv_t cd = iconv_open("ISO8859-1", "UTF-8");
    return Iconv(cd, input, input_size, buffer, buffer_size, error);
}

inline static size_t ConvertEucJpToUtf8(const char *input,
                                        size_t input_size,
                                        char *buffer,
                                        size_t buffer_size,
                                        std::error_code &error)
{
    static iconv_t cd = iconv_open("UTF-8", "EUC-JP");
    return Iconv(cd, input, input_size, buffer, buffer_size, error);
}

inline static size_t ConvertEucJpToUcs2(const char *input,
                                        size_t input_size,
                                        char *buffer,
                                        size_t buffer_size,
                                        std::error_code &error)
{
    static iconv_t cd = iconv_open("UCS-2", "EUC-JP");
    return Iconv(cd, input, input_size, buffer, buffer_size, error);
}

inline static size_t ConvertGb2312ToUcs2(const char *input,
                                         size_t input_size,
                                         char *buffer,
                                         size_t buffer_size,
                                         std::error_code &error)
{
    static iconv_t cd = iconv_open("UCS-2", "GB2312");
    return Iconv(cd, input, input_size, buffer, buffer_size, error);
}

inline static size_t ConvertIso8859_1ToUcs2(const char *input,
                                            size_t input_size,
                                            char *buffer,
                                            size_t buffer_size,
                                            std::error_code &error)
{
    static iconv_t cd = iconv_open("UCS-2", "ISO8859-1");
    return Iconv(cd, input, input_size, buffer, buffer_size, error);
}

static bool GuidToPosition(const char *guid,
                           EB_Position &out,
                           std::error_code &error)
{
    char *endptr;

    out.page = strtol(guid, &endptr, 10);
    if ((out.page == 0 && (errno == EINVAL || errno == ERANGE)) ||
        *endptr != ':') {
        error = make_error_code(simplify_error::bad_guid);
        return false;
    }

    out.offset = strtol(endptr + 1, &endptr, 10);
    if ((out.page == 0 && (errno == EINVAL || errno == ERANGE)) ||
        *endptr != '\0') {
        error = make_error_code(simplify_error::bad_guid);
        return false;
    }

    return true;
}

static size_t PositionToGuid(const EB_Position &pos,
                             char *buffer,
                             size_t buffer_size,
                             std::error_code &error)
{
    if (buffer_size < 21) {
        error = make_error_code(simplify_error::buffer_exhausted);
        return (size_t) -1;
    }

    assert(pos.page >= 0 && pos.offset >= 0);

    char *out = buffer;
    out += UIntToAlpha10(pos.page, out);
    *out++ = ':';
    out += UIntToAlpha10(pos.offset, out);
    *out = '\0';

    return out - buffer;
}

static v8::Handle<v8::Value> Print(const v8::Arguments &args)
{
    v8::HandleScope handle_scope;

    for (size_t i = 0; i < (size_t)args.Length(); i++) {
        v8::String::Utf8Value str(args[i]);
        std::cout << *str << " ";
    }

    std::cout << std::endl;
    return v8::Undefined();
}

enum class Charset {
    Iso8859_1,
    JisX0208,
    Gb2312,
    Ucs2
};

struct HookContext {
    struct ByteRange {
        Charset charset;
        size_t offset;
        size_t length;
    };

    std::vector<ByteRange> byte_ranges;
    size_t buffer_length;
    v8::Persistent<v8::Function> *js_functions;
};

class ExternalUcs2String : public v8::String::ExternalStringResource {
public:
    ExternalUcs2String(const uint16_t *string, size_t length)
      : string_(string),
        length_(length ) {
    }

    ~ExternalUcs2String() {
    }

    const uint16_t *data() const {
        return string_;
    }

    size_t length() const {
        return length_;
    }

private:
    const uint16_t *string_;
    size_t length_;
};

class EpwingDictionary::Private {
public:
    typedef EB_Error_Code (*reader_fun)(EB_Book *, EB_Appendix *, EB_Hookset *,
                                        void *, size_t, char *, ssize_t *);

    Private(EpwingDictionary *parent)
        : q(parent),
          last_sought_text_({-1, -1}) {

        eb_initialize_book(&book_);
        eb_initialize_hookset(&head_hookset_);
        eb_initialize_hookset(&text_hookset_);

        EB_Error_Code eb_code;

        // eb_set_hooks should not fail unless there is some invalid hook
        // code in the hook array.
        eb_code = eb_set_hooks(&head_hookset_, g_head_hooks);
        assert(eb_code == EB_SUCCESS);
        eb_code = eb_set_hooks(&text_hookset_, g_text_hooks);
        assert(eb_code == EB_SUCCESS);

        v8::Locker lock;
        v8::HandleScope handle_scope;
        v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();

        global->Set(v8::String::New("print"),
                    v8::FunctionTemplate::New(Print));

        js_context_ = v8::Context::New(NULL, global);
    }

    ~Private() {
        eb_finalize_hookset(&head_hookset_);
        eb_finalize_hookset(&text_hookset_);
        eb_finalize_book(&book_);

        for (size_t i = 0; i < g_js_function_count; ++i)
            js_functions_[i].Dispose();
        js_context_.Dispose();
    }

    /**
     * Binds the real dictionary located at @path to the current dictionary
     * object.
     */
    bool Bind(const char *path, std::error_code &error) {
        EB_Error_Code eb_code;

        eb_code = eb_bind(&book_, path);
        if (eb_code != EB_SUCCESS) {
            error = make_error_code(static_cast<eb_error>(eb_code));
            return false;
        }

        // Ensure that the dictionary uses one of the supported charsets.
        eb_code = eb_character_code(&book_, &charset_);
        if (eb_code == EB_SUCCESS) {
            if (charset_ != EB_CHARCODE_INVALID) {
                return true;
            } else {
                error = make_error_code(simplify_error::bad_charset);
                return false;
            }
        } else {
            error = make_error_code(static_cast<eb_error>(eb_code));
            return false;
        }
    }

    std::error_code PopulateJsContext() {
        // Read the user script from script.js.
        std::error_code error;
        std::string jscript =
            q->GetConfig().GetEnclosingDirectory() + "/custom.js";

        // Read file content.
        // We ignore any errors at this point because we still need to populate
        // context with default implementations of the hook functions.
        // In case of an error we'll just refuse to compile and execute user
        // script later.
        auto data = ReadFile(jscript.c_str(), error);

        // Populate JavaScript context of the dictionary.
        using namespace v8;

        Locker v8_lock;
        HandleScope handle_scope;
        Context::Scope context_scope(js_context_);
        Handle<Value> result;

        // Populate context with default implementations of the hook functions.
        // We allow calling default functions from the script.
        Persistent<Script> &default_script = GetDefaultJsScript();
        assert(!default_script.IsEmpty());
        result = default_script->Run();
        assert(!result.IsEmpty());

        // Compile and run user script if we have successfully read script
        // from the script.js file.
        if (!error) {
            Handle<String> source = String::New(data.first, data.second);
            Handle<String> origin = String::New(jscript.c_str());

            // TODO: Add more verbosiness to js error reports.
            Handle<Script> script = Script::Compile(source, origin);
            if (script.IsEmpty())
                error = make_error_code(simplify_error::js_compilation_error);

            result = script->Run();
            if (result.IsEmpty())
                error = make_error_code(simplify_error::js_runtime_error);

            for (size_t i = 0; i < g_js_function_count; ++i) {
                Handle<String> obj_name = String::New(g_js_function_names[i]);
                Handle<Value> obj = js_context_->Global()->Get(obj_name);

                if (!obj.IsEmpty() && obj->IsFunction())
                    js_functions_[i] =
                        Persistent<Function>::New(obj.As<Function>());
            }
        }

        // Check if we have any hook functions not supplied by the user script.
        std::string fun_name;
        for (size_t i = 0; i < g_js_function_count; ++i) {
            // Insert default implementation of the function if it's missing.
            if (js_functions_[i].IsEmpty()) {
                fun_name.clear();
                fun_name.append(1, '_') \
                        .append(g_js_function_names[i]);

                Handle<Value> fun =
                    js_context_->Global()->Get(String::New(fun_name.c_str()));
                assert(!fun.IsEmpty() && fun->IsFunction());

                js_functions_[i] =
                    Persistent<Function>::New(fun.As<Function>());
            }
        }

        free(data.first);

        return error;
    }

    bool SelectSubBook(size_t subbook_index, std::error_code &error) {
        EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
        int subbook_count;

        EB_Error_Code eb_code =
            eb_subbook_list(&book_, subbook_list, &subbook_count);

        if (eb_code != EB_SUCCESS) {
            error = make_error_code(static_cast<eb_error>(eb_code));
            return false;
        }

        if (subbook_index < (size_t)subbook_count) {
            eb_code = eb_set_subbook(&book_, subbook_list[subbook_index]);

            if (eb_code == EB_SUCCESS) {
                return true;
            } else {
                error = make_error_code(static_cast<eb_error>(eb_code));
                return false;
            }
        } else {
            error = make_error_code(simplify_error::index_out_of_range);
            return false;
        }
    }

    inline bool SeekEntity(EB_Position &position, std::error_code &e) {
        EB_Error_Code eb_code = eb_seek_text(&book_, &position);

        if (eb_code == EB_SUCCESS) {
            return true;
        } else {
            e = make_error_code(static_cast<eb_error>(eb_code));
            return false;
        }
    }

    inline bool SeekTextConvenient(EB_Position &position, std::error_code &e) {
        if (position.page != last_sought_text_.page ||
            position.offset != last_sought_text_.offset) {
            return SeekEntity(position, e);
        } else {
            return true;
        }
    }

    size_t ReadUcs2Text(reader_fun function,
                        EB_Hookset &hookset,
                        char *buffer,
                        size_t buffer_size,
                        std::error_code &error) {

        assert(buffer_size > 0);

        char tmp_buffer[buffer_size];
        ssize_t text_length;

        v8::HandleScope handle_scope;
        v8::Context::Scope context_scope(js_context_);

        struct HookContext write_context;
        write_context.byte_ranges.reserve(6);
        write_context.buffer_length = 0;
        write_context.js_functions = js_functions_;

        // Read dictionary text into temporary buffer. The buffer might
        // contain char ranges with different character encodings. Each
        // range is described in the HookContext::byte_ranges vector by
        // hook functions. Later, we'll encode bufer into UTF-8.
        EB_Error_Code eb_code = (*function)(&book_,
                                            NULL,
                                            &hookset,
                                            &write_context,
                                            sizeof(tmp_buffer) - 1,
                                            tmp_buffer,
                                            &text_length);
        if (eb_code != EB_SUCCESS) {
            error = make_error_code(static_cast<eb_error>(eb_code));
            return -1;
        }

        if (!eb_is_text_stopped(&book_)) {
            error = make_error_code(simplify_error::buffer_exhausted);
            return -1;
        }

        // Re-encode non-uniform temporary buffer into UTF-8 string and
        // store it into the given buffer.
        size_t result_length = 0;

        for (auto it = write_context.byte_ranges.begin();
             it != write_context.byte_ranges.end();
             ++it) {

            size_t length;

            switch ((*it).charset) {
                case Charset::Iso8859_1: {
                    length = ConvertIso8859_1ToUcs2(
                            tmp_buffer + (*it).offset,
                            (*it).length,
                            buffer + result_length,
                            buffer_size - result_length,
                            error);
                    break;
                }
                case Charset::JisX0208: {
                    length = ConvertEucJpToUcs2(
                            tmp_buffer + (*it).offset,
                            (*it).length,
                            buffer + result_length,
                            buffer_size - result_length,
                            error);
                    break;
                }
                case Charset::Gb2312: {
                    length = ConvertGb2312ToUcs2(
                            tmp_buffer + (*it).offset,
                            (*it).length,
                            buffer + result_length,
                            buffer_size - result_length,
                            error);
                    break;
                }
                case Charset::Ucs2: {
                    // Copy the UCS-2 range into the dst. buffer verbatim.
                    length = (*it).length;

                    // Ensure if there is enough free space in the buffer.
                    // NOTE: We also fail if the resulting length is equal
                    // to the buffer size. Even though, we might have enough
                    // space to store this UCS-2 string, it will certainly not
                    // enough to store terminating NULL byte.
                    if (result_length + length >= buffer_size) {
                        error =
                            make_error_code(simplify_error::buffer_exhausted);
                        return -1;
                    }

                    memcpy(buffer + result_length,
                           tmp_buffer + (*it).offset,
                           length);
                    break;
                }
            }

            if (!error)
                result_length += length;
            else
                return -1;
        }

        assert(result_length % 2 == 0);
        return result_length;
    }


    size_t PipeStringThroughJsFunction(JsFunction function,
                                       const char *data,
                                       size_t data_length,
                                       char *buffer,
                                       size_t buffer_size,
                                       std::error_code &error) {

        // Pipe the article's heading through the user script.
        using namespace v8;

        HandleScope handle_scope;
        Context::Scope context_scope(js_context_);

        size_t fun_index = static_cast<size_t>(function);
        Handle<Value> argv[] = {
            String::NewExternal(
                    new ExternalUcs2String(
                        reinterpret_cast<const uint16_t *>(data),
                        data_length / 2))
        };

        Handle<Value> result = js_functions_[fun_index]->Call(
                v8::Context::GetEntered()->Global(),
                sizeof(argv) / sizeof(argv[0]),
                argv);

        // Copy the result into the buffer.
        Handle<String> string;
        int nchars;
        int nbytes;

        // Ensure that the js function has returned a valid string object.
        // It's very unlikely that the function will return an invalid
        // object, but if it does, that means that the function contains a
        // bug. In this case we put into the buffer an unmodified dictionary
        // data.
        if (!result.IsEmpty() && result->IsString()) {
            string = result.As<String>();
        } else {
            //string = argv[0].As<String>();
            error = make_error_code(simplify_error::unexpected_result_type);
            return -1;
        }

        nbytes = string->WriteUtf8(buffer, buffer_size, &nchars);

        // Check if we have enough space in the buffer to copy the result.
        if (nchars == string->Length()) {
            // We return the length of the result, but nbytes includes 1 byte
            // for NULL terminator.
            return nbytes - 1;
        } else {
            error = make_error_code(simplify_error::buffer_exhausted);
            return -1;
        }
    }

    inline size_t ReadHeading(char *buffer, size_t size, std::error_code &e) {
        return ReadUcs2Text(&eb_read_heading, head_hookset_, buffer, size, e);
    }

    inline size_t ReadText(char *buffer, size_t size, std::error_code &e) {
        return ReadUcs2Text(&eb_read_text, text_hookset_, buffer, size, e);
    }

public:
    EpwingDictionary *q;
    EB_Book book_;
    EB_Character_Code charset_;
    EB_Hookset head_hookset_;
    EB_Hookset text_hookset_;

    EB_Position last_sought_text_;

    v8::Persistent<v8::Context> js_context_;
    v8::Persistent<v8::Function> js_functions_[g_js_function_count];
};

class EbSearchResults : public Dictionary::SearchResults {
public:
    EbSearchResults(EpwingDictionary::Private *p,
                    size_t hit_count,
                    EB_Hit *hits)
      : d(p),
        hit_offset_(-1),
        hit_count_(hit_count),
        hits_(hits)
    {
    }

    size_t GetCount() const {
        return hit_count_;
    }

    bool SeekNext() {
        size_t offset = hit_offset_ + 1;
        std::error_code e;

        if (unlikely(offset >= hit_count_ ||
                     !d->SeekEntity(hits_[offset].heading, e)))
            return false;

        hit_offset_ = offset;

        if (offset > 0) {
            EB_Position &prevText = hits_[offset - 1].text;
            EB_Position &thisText = hits_[offset].text;

            // For some reason, some dictionaries return two entries
            // pointing to the same article. It wont be a problem
            // if there would be only a few duplicates, but in reality,
            // search results tend to be oversaturated with duplicates.
            //
            // It is also worth pointing out that checking each entry
            // against hash table would be more bullet-proof method,
            // but I haven't yet encountered any non-adjacent duplicates.
            return unlikely(
                    prevText.page == thisText.page &&
                    prevText.offset == thisText.offset) ? SeekNext() : true;
        } else {
            return true;
        }
    }

    Likely<size_t> InitializeHeadingData(char *buffer, size_t buffer_size) {
        std::error_code error;
        size_t length = d->ReadHeading(buffer, buffer_size, error);

        if (length != (size_t) -1)
            return length;
        else
            return error;
    }

    Likely<size_t> FetchHeading(const char *data,
                                size_t data_length,
                                char *buffer,
                                size_t buffer_size) {
        std::error_code error;
        size_t length = d->PipeStringThroughJsFunction(
                JsFunction::ProcessHeading,
                data,
                data_length,
                buffer,
                buffer_size,
                error);

        if (length != (size_t)-1)
            return length;
        else
            return error;
    }

    Likely<size_t> FetchTags(const char *data,
                             size_t data_length,
                             char *buffer,
                             size_t buffer_size) {
        std::error_code error;
        size_t length = d->PipeStringThroughJsFunction(
                JsFunction::ProcessTags,
                data,
                data_length,
                buffer,
                buffer_size,
                error);

        if (length != (size_t)-1)
            return length;
        else
            return error;
    }

    Likely<size_t> FetchGuid(char *buffer, size_t buffer_size) {
        std::error_code error;
        size_t length = PositionToGuid(hits_[hit_offset_].text,
                                       buffer,
                                       buffer_size,
                                       error);
        if (length != (size_t)-1)
            return length;
        else
            return error;
    }

    inline void *operator new(size_t size, void *mem) throw() {
        return mem;
    }

    inline void operator delete(void *mem) {
        free(mem);
    }

private:
    EpwingDictionary::Private *d;
    size_t hit_offset_;
    size_t hit_count_;
    EB_Hit *hits_;

    v8::Locker v8_lock_;
};

EpwingDictionary::EpwingDictionary(Config *conf) : Dictionary(conf)
{
    // Instantiate Locker before doing anything JS related to let v8 know
    // that we might be using threads.
    v8::Locker lock;

    d = new Private(this);
}

EpwingDictionary::~EpwingDictionary()
{
    delete d;
}

Likely<std::vector<std::string>> EpwingDictionary::ListSubBooks() const
{
    EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
    int subbook_count;

    int error = eb_subbook_list(&d->book_, subbook_list, &subbook_count);
    if (error != EB_SUCCESS)
        return make_error_code(static_cast<eb_error>(error));

    std::vector<std::string> names_list;

    for (int i = 0; i < subbook_count; ++i) {
        char buffer[EB_MAX_TITLE_LENGTH + 1];
        char utf8buffer[EB_MAX_TITLE_LENGTH * 6 + 1];
        EB_Error_Code eb_code =
            eb_subbook_title2(&d->book_, subbook_list[i], buffer);

        // Errors while working with subbooks are not critical.
        // If an error occurs we just insert some dummy title.
        if (eb_code != EB_SUCCESS) {
            names_list.push_back("<Error while reading book title>");
            continue;
        }

        // FIXME: I'm not sure what encoding subbook titles use.
        // Assuming EUC-JP.
        std::error_code error;
        size_t length = ConvertEucJpToUtf8(buffer,
                                           strlen(buffer),
                                           utf8buffer,
                                           sizeof(utf8buffer),
                                           error);
        if (!error)
            names_list.push_back(std::string(utf8buffer, length));
        else
            names_list.push_back("<EUC-JP to UTF-8 conversion error>");
    }

    return names_list;
}

std::error_code EpwingDictionary::SelectSubBook(size_t subbook_index) {
    std::error_code error;

    if (d->SelectSubBook(subbook_index, error)) {
        // Save the preference so we can restore last selected
        // subbook the next time the dictionary is created.
        (*conf_)["Epwing"].WriteInt32("subbook", subbook_index);
    }

    return error;
}

Likely<Dictionary::SearchResults *> EpwingDictionary::Search(const char *expr,
                                                             size_t limit)
{
    size_t expr_length = strlen(expr);

    // Don't try if the search expression is too long.
    if (expr_length > EB_MAX_WORD_LENGTH)
        return make_error_code(simplify_error::search_expr_too_long);

    // Re-encode search string to the string with character encoding
    // required by the dictionary.
    char conv_expr[expr_length * 3 + 1];
    std::error_code last_error;

    if (d->charset_ != EB_CHARCODE_ISO8859_1) {
        ConvertUtf8ToEucJp(expr,
                           expr_length + 1,
                           conv_expr,
                           sizeof(conv_expr),
                           last_error);
    } else {
        ConvertUtf8ToIso8859_1(expr,
                               expr_length + 1,
                               conv_expr,
                               sizeof(conv_expr),
                               last_error);
    }

    if (last_error) return last_error;

    // Check if we were requested to do a suffix search.
    // Accepted wildcards are '*', '＊' (U+FF0A), '_' and '＿' (U+FF3F).
    if (expr[0] == '*' || expr[0] == '_' || (expr_length > 1 &&
        (memcmp(expr, "\xff\x0a", 2) == 0 ||
         memcmp(expr, "\xff\x3f", 2) == 0))) {

        if (eb_have_endword_search(&d->book_)) {
            // TODO: Implement suffix search (need a dictionary that supports
            // this method).
            std::cout << "TODO: Suffix search" << std::endl;
        } else {
            return make_error_code(simplify_error::no_suffix_search);
        }
    }

    // Since we weren't requested to use any kind of special search method,
    // try to find a search method that's supported by the dictionary.
    EB_Error_Code (*search_fun)(EB_Book *, const char *);

    if (eb_have_word_search(&d->book_))
        search_fun = &eb_search_word;
    else if (eb_have_exactword_search(&d->book_))
        search_fun = &eb_search_exactword;
    else
        return make_error_code(simplify_error::cant_search);

    // Perform search using selected search method.
    EB_Error_Code eb_code = (*search_fun)(&d->book_, conv_expr);
    if (eb_code != EB_SUCCESS)
        return make_error_code(static_cast<eb_error>(eb_code));

    return GetResults(limit);
}

Likely<size_t> EpwingDictionary::ReadText(const char *guid, char **ptr)
{
    EB_Position position;
    std::error_code last_error;

    if (!GuidToPosition(guid, position, last_error) ||
        !d->SeekTextConvenient(position, last_error)) {
        *ptr = NULL;
        return last_error;
    }

    // Acquire big v8 lock (a requiremenet imposed by ReadText and
    // PipeStringThroughJsFunction methods).
    v8::Locker v8_lock;

    size_t tmp_buffer_size = 4096;

    // Read text from the dictionary. For the reason that libeb doesn't
    // provide any way to discover the length of the article's text,
    // we need to guess an optimal size of the buffer by using bruteforce.
    while (true) {
        char tmp_buffer[tmp_buffer_size];
        size_t length = d->ReadText(tmp_buffer, tmp_buffer_size, last_error);

        if (length != (size_t) -1) {
            // Text read successfully.

            // Allocate large enough buffer store store UTF-8 text.
            size_t buffer_size = length * 2;
            *ptr = new char[buffer_size];

            length = d->PipeStringThroughJsFunction(JsFunction::ProcessText,
                                                    tmp_buffer,
                                                    length,
                                                    *ptr,
                                                    buffer_size,
                                                    last_error);
            if (length != (size_t)-1) {
                return length;
            } else {
                delete[] *ptr;
                return last_error;
            }
        } else {
            // Do another round if the buffer was not big enough.
            if (last_error == simplify_error::buffer_exhausted) {
                tmp_buffer_size += 2048;
                continue;
            } else {
                return last_error;
            }
        }
    }
}

Likely<size_t> EpwingDictionary::ReadText(const char *guid,
                                          char *buffer,
                                          size_t buffer_size)
{
    EB_Position position;
    std::error_code last_error;

    if (!GuidToPosition(guid, position, last_error) ||
        !d->SeekTextConvenient(position, last_error)) {
        return last_error;
    }

    // Acquire big v8 lock (a requiremenet imposed by ReadText and
    // PipeStringThroughJsFunction methods).
    v8::Locker v8_lock;

    // We need a temporary buffer to store the UCS-2 text.
    char tmp_buffer[buffer_size];

    size_t length = d->ReadText(tmp_buffer, buffer_size, last_error);
    if (length == (size_t)-1)
        return last_error;

    length = d->PipeStringThroughJsFunction(JsFunction::ProcessText,
                                            tmp_buffer,
                                            length,
                                            buffer,
                                            buffer_size,
                                            last_error);
    if (length != (size_t)-1)
        return length;
    else
        return last_error;
}

Likely<Dictionary::SearchResults *> EpwingDictionary::GetResults(
                                                             size_t max_count)
{
    EB_Error_Code eb_code;
    size_t total_count = 0;
    int increase_step = 256;

    struct SearchResultsMemoryLayout {
        uint8_t results[sizeof(EbSearchResults)];
        EB_Hit hits[];
    } *sr = NULL;

    while (true) {
        int step = std::min(max_count - total_count, (size_t)increase_step);

        sr = static_cast<SearchResultsMemoryLayout *>(
                realloc(sr, sizeof(SearchResultsMemoryLayout) + \
                            total_count * sizeof(EB_Hit)      + \
                            step * sizeof(EB_Hit)));

        int hit_count;
        eb_code = eb_hit_list(&d->book_,
                              step,
                              sr->hits + total_count,
                              &hit_count);

        if (eb_code == EB_SUCCESS) {
            total_count += hit_count;

            // Be done, if the number of returned hits is less than the number
            // of items we've allocated or we've reached maximum number of
            // results.
            if (hit_count < step || total_count >= max_count)
                break;
        } else {
            return make_error_code(static_cast<eb_error>(eb_code));
        }
    }

    SearchResults *results =
        new(sr->results) EbSearchResults(d, total_count, sr->hits);
    return results;
}

Likely<EpwingDictionary *> EpwingDictionary::New(const char *path,
                                                 Config *conf)
{
    // Write the path of the dictionary in advance so the Initialize() method
    // will be able to pick it up.
    (*conf)["Epwing"].WriteString("path", path);

    EpwingDictionary *dict = new EpwingDictionary(conf);
    std::error_code error = dict->Initialize();

    if (!error)
        return dict;
    else
        return error;
}

Likely<EpwingDictionary *> EpwingDictionary::New(Config *conf)
{
    EpwingDictionary *dict = new EpwingDictionary(conf);
    std::error_code error = dict->Initialize();

    if (!error)
        return dict;
    else
        return error;
}

std::error_code EpwingDictionary::Initialize()
{
    const char *path;
    std::error_code last_error;

    if (!InitializeLibEb(last_error))
        return last_error;

    Config::ConfigSection &epwing = conf_->GetSection("Epwing");
    bool success = epwing.ReadString("path", &path);

    if (!success)
        return make_error_code(simplify_error::bad_configuration);

    if (!d->Bind(path, last_error))
        return last_error;

    // TODO: Notify if we've failed to select subbook.
    d->SelectSubBook(epwing.ReadInt32("subbook", 0), last_error);

    // TODO: Notify if we've failed to load user scripts.
    d->PopulateJsContext();
    return last_error;
}

inline static EB_Error_Code WriteByteRange(EB_Book *book,
                                           HookContext *context,
                                           Charset charset,
                                           const char *string,
                                           size_t length)
{
    if (length == 0) {
        return EB_SUCCESS;
    }

    // Start new byte range descriptor if the given string doesn't have the
    // same charset as the charset of the previous byte range.
    if (context->byte_ranges.empty() ||
        context->byte_ranges.back().charset != charset) {

        context->byte_ranges.push_back(HookContext::ByteRange());

        HookContext::ByteRange &byte_range = context->byte_ranges.back();
        byte_range.charset = charset;
        byte_range.offset = context->buffer_length;
    }

    EB_Error_Code eb_code = eb_write_text(book, string, length);

    if (eb_code == EB_SUCCESS) {
        context->byte_ranges.back().length += length;
        context->buffer_length += length;
    }

    return eb_code;
}

inline static EB_Error_Code WriteJs(EB_Book *book,
                                    HookContext *context,
                                    JsFunction function,
                                    int argc,
                                    v8::Handle<v8::Value> *argv)
{
    size_t fun_index = static_cast<size_t>(function);

    assert(!context->js_functions[fun_index].IsEmpty() &&
           context->js_functions[fun_index]->IsFunction());

    using namespace v8;
    Handle<Value> result =
        context->js_functions[fun_index]->Call(
                Context::GetEntered()->Global(),
                argc,
                argv);

    if (!result.IsEmpty() && result->IsString()) {
        Handle<String> string = result.As<String>();
        uint16_t chars[string->Length() + 1];
        size_t copied = string->Write(chars);

        // Even though the v8 documentation states that the value returned
        // from the String::Write() method is the number of bytes written,
        // in reality though, it's the number of characters written.
        return WriteByteRange(book,
                              context,
                              Charset::Ucs2,
                              reinterpret_cast<char *>(chars),
                              copied * sizeof(uint16_t));
    } else {
        return EB_SUCCESS;
    }
}

inline static EB_Error_Code WriteJs(EB_Book *book,
                                    HookContext *context,
                                    JsFunction function)
{
    return WriteJs(book, context, function, 0, NULL);
}

static EB_Error_Code HandleIso8859_1(EB_Book *book, EB_Appendix *, void *arg,
                                     EB_Hook_Code, int argc,
                                     const unsigned int *argv)
{
    // FIXME: Broken on big-endian machines.
    return WriteByteRange(book,
                          reinterpret_cast<HookContext *>(arg),
                          Charset::Iso8859_1,
                          reinterpret_cast<const char *>(&argv[0]),
                          1);
}

static EB_Error_Code HandleJisX0208(EB_Book *book, EB_Appendix *, void *arg,
                                    EB_Hook_Code, int argc,
                                    const unsigned int *argv)
{
    // FIXME: Broken on big-endian machines.
    unsigned int c = __builtin_bswap32(argv[0]);
    size_t size = ((c & 0xff000000) != 0) + ((c & 0x00ff0000) != 0) + \
                  ((c & 0x0000ff00) != 0) + ((c & 0x000000ff) != 0);

    return WriteByteRange(book,
                          reinterpret_cast<HookContext *>(arg),
                          Charset::JisX0208,
                          reinterpret_cast<const char *>(&c) + sizeof(c) - size,
                          size);
}

static EB_Error_Code HandleGb2312(EB_Book *book, EB_Appendix *, void *arg,
                                  EB_Hook_Code, int argc,
                                  const unsigned int *argv)
{
    // FIXME: Broken on big-endian machines.
    unsigned int c = __builtin_bswap32(argv[0]);
    size_t size = ((c & 0xff000000) != 0) + ((c & 0x00ff0000) != 0) + \
                  ((c & 0x0000ff00) != 0) + ((c & 0x000000ff) != 0);

    return WriteByteRange(book,
                          reinterpret_cast<HookContext *>(arg),
                          Charset::Gb2312,
                          reinterpret_cast<const char *>(&c) + sizeof(c) - size,
                          size);
}

static EB_Error_Code HandleBeginSub(EB_Book *book, EB_Appendix *, void *arg,
                                    EB_Hook_Code, int argc,
                                    const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::BeginSubscript);
}

static EB_Error_Code HandleEndSub(EB_Book *book, EB_Appendix *, void *arg,
                                  EB_Hook_Code, int argc,
                                  const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::EndSubscript);
}

static EB_Error_Code HandleBeginSup(EB_Book *book, EB_Appendix *, void *arg,
                                    EB_Hook_Code, int argc,
                                    const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::BeginSuperscript);
}

static EB_Error_Code HandleEndSup(EB_Book *book, EB_Appendix *, void *arg,
                                  EB_Hook_Code, int argc,
                                  const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::EndSuperscript);
}

static EB_Error_Code HandleIndent(EB_Book *book, EB_Appendix *, void *arg,
                                  EB_Hook_Code, int argc,
                                  const unsigned int *argv)
{
    v8::Handle<v8::Value> v8argv[] = { v8::Uint32::New(argv[1]) };
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::Indent,
                   sizeof(v8argv) / sizeof(v8argv[0]),
                   v8argv);
}

static EB_Error_Code HandleNewline(EB_Book *book, EB_Appendix *, void *arg,
                                   EB_Hook_Code, int argc,
                                   const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::Newline);
}

static EB_Error_Code HandleBeginNoBr(EB_Book *book, EB_Appendix *, void *arg,
                                     EB_Hook_Code, int argc,
                                     const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::BeginNoBreak);
}

static EB_Error_Code HandleEndNoBr(EB_Book *book, EB_Appendix *, void *arg,
                                   EB_Hook_Code, int argc,
                                   const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::EndNoBreak);
}

static EB_Error_Code HandleBeginEm(EB_Book *book, EB_Appendix *, void *arg,
                                   EB_Hook_Code, int argc,
                                   const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::BeginEmphasis);
}

static EB_Error_Code HandleEndEm(EB_Book *book, EB_Appendix *, void *arg,
                                 EB_Hook_Code, int argc,
                                 const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::EndEmphasis);
}

static EB_Error_Code HandleBeginReference(EB_Book *book, EB_Appendix *,
                                          void *arg, EB_Hook_Code, int argc,
                                          const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::BeginReference);
}

static EB_Error_Code HandleEndReference(EB_Book *book, EB_Appendix *,
                                        void *arg, EB_Hook_Code, int argc,
                                        const unsigned int *argv)
{
    v8::Handle<v8::Value> v8argv[] = {
        v8::Int32::New(argv[1]),
        v8::Int32::New(argv[2])
    };

    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::Indent,
                   sizeof(v8argv) / sizeof(v8argv[0]),
                   v8argv);
}

static EB_Error_Code HandleBeginKeyword(EB_Book *book, EB_Appendix *,
                                        void *arg, EB_Hook_Code, int argc,
                                        const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::BeginKeyword);
}

static EB_Error_Code HandleEndKeyword(EB_Book *book, EB_Appendix *, void *arg,
                                      EB_Hook_Code, int argc,
                                      const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::EndKeyword);
}

static EB_Error_Code HandleBeginDecoration(EB_Book *book, EB_Appendix *,
                                           void *arg, EB_Hook_Code, int argc,
                                           const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::BeginDecoration);
}

static EB_Error_Code HandleEndDecoration(EB_Book *book, EB_Appendix *,
                                         void *arg, EB_Hook_Code, int argc,
                                         const unsigned int *argv)
{
    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::EndDecoration);
}

static EB_Error_Code HandleInsertHGaiji(EB_Book *book, EB_Appendix *,
                                        void *arg, EB_Hook_Code, int argc,
                                        const unsigned int *argv)
{
    v8::Handle<v8::Value> v8argv[] = {
        v8::Uint32::New(argv[0])
    };

    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::InsertHeadingGaiji,
                   sizeof(v8argv) / sizeof(v8argv[0]),
                   v8argv);
}

static EB_Error_Code HandleInsertTGaiji(EB_Book *book, EB_Appendix *,
                                        void *arg, EB_Hook_Code, int argc,
                                        const unsigned int *argv)
{
    v8::Handle<v8::Value> v8argv[] = {
        v8::Uint32::New(argv[0])
    };

    return WriteJs(book,
                   reinterpret_cast<HookContext *>(arg),
                   JsFunction::InsertTextGaiji,
                   sizeof(v8argv) / sizeof(v8argv[0]),
                   v8argv);
}

}  // namespace simplify
