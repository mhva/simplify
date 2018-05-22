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
#include <memory>
#include <sstream>
#include <string>

#include <eb/eb.h>
#include <eb/error.h>
#include <eb/text.h>

#include <v8.h>

#include <nowide/fstream.hpp>
#include <nlohmann/json.hpp>

#include <simplify/error.hh>
#include <simplify/utils.hh>

#include "eucjp_ucs2.hh"
#include "defaultjs.hh"
#include "epwing-dictionary.hh"

#define ENTER_ISOLATE(isolate)                     \
  auto isolate__ = isolate;                        \
  ::v8::Locker isolate_locker__(isolate__);        \
  ::v8::Isolate::Scope isolate_scope__(isolate__); \
  ::v8::HandleScope handle_scope__(isolate__);
#define ENTER_CONTEXT(context)                     \
  (void) isolate__;                                \
  ::v8::Local<v8::Context> js_context__ = context; \
  ::v8::Context::Scope js_context_scope__{js_context__};
#define GET_CONTEXT() js_context__

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

static v8::MaybeLocal<v8::Script> CompileBuiltinScript(
                                      v8::Local<v8::Context> context)
{
    v8::MaybeLocal<v8::String> source = v8::String::NewFromUtf8(
        context->GetIsolate(),
        g_default_js_implementation,
        v8::NewStringType::kNormal
      );
    if (!source.IsEmpty())
        return v8::Script::Compile(context, source.ToLocalChecked());
    else
        return v8::Local<v8::Script>();
}

/**
 * Converts the encoding of source string using iconv conversion descriptor
 * \p cd.
 *
 * \param cd iconv conversion descriptor.
 * \param input A pointer to a source string to encode.
 * \param input_size Size of the \p input string.
 * \param buffer A pointer to the buffer where resulting string should be
 *  stored.
 * \param buffer_size Size of the buffer pointed by \p buffer.
 * \param error Reference to std::error_code where an error, if any, will be
 *  stored. If encoding succeeds the error state will be cleared.
 *
 * \return If succeeds, returns number of bytes in the \p buffer string.
 * If fails, <em>(size_t) -1</em> will be returned.
 */
static size_t Iconv(iconv_t cd, const char *input, size_t input_size,
                    char *buffer, size_t buffer_size, std::error_code &error)
{
    error.clear();

    if (cd == (iconv_t) -1) {
        error = make_error_code(static_cast<std::errc>(errno));
        return (size_t) -1;
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

        return (size_t) -1;
    }
}

/**
 * Encodes UTF-8 string \p input to EUC-JP and writes the result into the
 * \p buffer with the max size of \p buffer_size bytes.
 */
inline static size_t ConvertUtf8ToEucJp(const char *input, size_t input_size,
                                        char *buffer, size_t buffer_size,
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

inline static size_t ConvertEucJpToUtf8(const char *input, size_t input_size,
                                        char *buffer, size_t buffer_size,
                                        std::error_code &error)
{
    static iconv_t cd = iconv_open("UTF-8", "EUC-JP");
    return Iconv(cd, input, input_size, buffer, buffer_size, error);
}

inline static size_t ConvertEucJpToUcs2(const char *input, size_t input_size,
                                        char *buffer, size_t buffer_size,
                                        std::error_code &error)
{
    static iconv_t cd = iconv_open("UCS-2", "EUC-JP");
    return Iconv(cd, input, input_size, buffer, buffer_size, error);
}

inline static size_t ConvertGb2312ToUcs2(const char *input, size_t input_size,
                                         char *buffer, size_t buffer_size,
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

static bool GuidToPosition(const char *guid, EB_Position &out,
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
    if (buffer_size < 22) {
        error = make_error_code(simplify_error::buffer_exhausted);
        return (size_t) -1;
    }

    assert(pos.page >= 0 && pos.offset >= 0);

    char *out = buffer;
    out += UIntToAlpha10(static_cast<size_t>(pos.page), out);
    *out++ = ':';
    out += UIntToAlpha10(static_cast<size_t>(pos.offset), out);
    *out = '\0';

    return out - buffer;
}

static void Print(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    for (int i = 0; i < args.Length(); i++) {
        v8::String::Utf8Value str(args[i]);
        std::cout << *str << " ";
    }

    std::cout << std::endl;
}

enum class Charset {
    Iso8859_1,
    JisX0208,
    Gb2312,
    Ucs2
};

class ExternalUcs2String : public v8::String::ExternalStringResource {
public:
    ExternalUcs2String(std::shared_ptr<uint16_t> string, size_t length)
      : string_(string),
        length_(length) {}

    const uint16_t *data() const override { return string_.get(); }
    size_t length() const override { return length_; }

protected:
    void Dispose() override { delete this; }

private:
    std::shared_ptr<uint16_t> string_;
    size_t length_;
};

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
 public:
  virtual ~ArrayBufferAllocator() = default;
  virtual void* Allocate(size_t length) { return calloc(1, length); }
  virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
  virtual void Free(void* data, size_t) { free(data); }
};

class EpwingDictionary::Private {
public:
    typedef EB_Error_Code (*ReaderFn)(EB_Book *, EB_Appendix *, EB_Hookset *,
                                      void *, size_t, char *, ssize_t *);

    Private()
      : last_sought_text_({-1, -1})
      , current_subbook_(0)
      , isolate_(nullptr, [](v8::Isolate *p) { p->Dispose(); }) {
        eb_initialize_book(&book_);
        eb_initialize_hookset(&head_hookset_);
        eb_initialize_hookset(&text_hookset_);

        EB_Error_Code eb_code;

        // eb_set_hooks should not fail unless there is a hook with an invalid
        // id in the hook list.
        eb_code = eb_set_hooks(&head_hookset_, g_head_hooks);
        assert(eb_code == EB_SUCCESS);
        eb_code = eb_set_hooks(&text_hookset_, g_text_hooks);
        assert(eb_code == EB_SUCCESS);

        v8::Isolate::CreateParams create_params;
        create_params.array_buffer_allocator = &array_buffer_allocator_;
        isolate_.reset(v8::Isolate::New(create_params));

        v8::Locker isolate_locker(isolate_.get());
        v8::Isolate::Scope isolate_scope(isolate_.get());
        v8::HandleScope handle_scope(isolate_.get());

        v8::Local<v8::ObjectTemplate> root_template =
            v8::ObjectTemplate::New(isolate_.get());
        root_template->Set(
            v8::String::NewFromUtf8(isolate_.get(), "print",
                                    v8::NewStringType::kNormal
                                   ).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate_.get(), Print));

        js_context_handle_.Reset(isolate_.get(),
            v8::Context::New(isolate_.get(), nullptr, root_template));
    }

    ~Private() {
        eb_finalize_hookset(&head_hookset_);
        eb_finalize_hookset(&text_hookset_);
        eb_finalize_book(&book_);

        //for (size_t i = 0; i < g_js_function_count; ++i)
        //    js_functions_[i].Reset();
        //js_context_handle_.Reset();
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

    std::error_code PopulateJsContext(const char *script_filename) {
        // Read the user script from script.js.
        std::error_code error;

        script_path_ = script_filename;

        // Read file content.
        // We ignore any errors at this point because we still need to populate
        // context with default implementations of the hook functions.

        std::string custom_script;

        if (script_filename != nullptr && strlen(script_filename) > 0) {
            nowide::ifstream stream(script_filename);

            if (stream) {
                try {
                    std::stringstream ss;
                    ss << stream.rdbuf();
                    custom_script = ss.str();
                } catch (...) {
                    // FIXME: report failure.
                }
            } else {
                // FIXME: report failure.
            }
        }

        using namespace v8;

        ENTER_ISOLATE(isolate_.get());
        ENTER_CONTEXT(GetJsContext());
        Local<Context> &context = GET_CONTEXT();

        // Populate JS environment with the default implementation of
        // callback functions.
        MaybeLocal<Value> maybe_result;
        MaybeLocal<Script> environ_script = CompileBuiltinScript(context);
        if (environ_script.IsEmpty())
            return make_error_code(simplify_error::js_compilation_error);
        maybe_result = environ_script.ToLocalChecked()->Run(context);
        assert(!maybe_result.IsEmpty());

        // Compile and run user script to populate current javascript context
        // with user callbacks.
        if (custom_script.length() > 0) {
          MaybeLocal<String> source =
              String::NewFromUtf8(isolate_.get(), custom_script.c_str(),
                                  NewStringType::kNormal,
                                  static_cast<int>(custom_script.length()));
          MaybeLocal<String> source_filename =
              String::NewFromUtf8(isolate_.get(), script_filename,
                                  NewStringType::kNormal);
          // TODO: More verbose error reports.
          if (source_filename.IsEmpty())
              return make_error_code(simplify_error::js_allocation_error);

          ScriptOrigin origin{source_filename.ToLocalChecked()};
          MaybeLocal<Script> maybe_user_script =
              Script::Compile(context, source.ToLocalChecked(), &origin);
          if (maybe_user_script.IsEmpty())
              return make_error_code(simplify_error::js_compilation_error);

          maybe_result = maybe_user_script.ToLocalChecked()->Run();
          if (maybe_result.IsEmpty())
              return make_error_code(simplify_error::js_runtime_error);
        }

        // Resolve callbacks and save their handles for ease of access later.
        std::string fallback_name;
        for (size_t i = 0; i < g_js_function_count; ++i) {
            MaybeLocal<String> obj_name = String::NewFromUtf8(
                isolate_.get(),
                g_js_function_names[i],
                NewStringType::kNormal
              );
            MaybeLocal<Value> obj = context->Global()->Get(
                context,
                obj_name.ToLocalChecked()
              );


            if (!obj.IsEmpty() && obj.ToLocalChecked()->IsFunction()) {
                // TODO: print an error, if the object is not a function.
                Local<Function> fn = obj.ToLocalChecked().As<Function>();
                js_functions_[i].Reset(isolate_.get(), fn);
            } else {
                // User script does not implement this callback, fall back
                // to using its default implementation.

                // Default implementation can be resolved by appending
                // an underscore to callback name.
                fallback_name.clear();
                fallback_name.append(1, '_').append(g_js_function_names[i]);

                MaybeLocal<String> maybe_fallback_name =
                    String::NewFromUtf8(isolate_.get(), fallback_name.c_str(),
                                        NewStringType::kNormal);
                if (maybe_fallback_name.IsEmpty())
                    return make_error_code(simplify_error::js_allocation_error);

                Local<String> v8_fallback_name =
                    maybe_fallback_name.ToLocalChecked();
                MaybeLocal<Value> maybe_fn_handle =
                    context->Global()->Get(context, v8_fallback_name);

                assert(!maybe_fn_handle.IsEmpty());
                Local<Value> fn_handle = maybe_fn_handle.ToLocalChecked();
                assert(fn_handle->IsFunction());

                js_functions_[i].Reset(isolate_.get(),
                                       fn_handle.As<Function>());
            }
        }

        return make_error_code(simplify_error::success);
    }

    bool SelectSubBook(int subbook_index, std::error_code &error) {
        EB_Subbook_Code subbook_list[EB_MAX_SUBBOOKS];
        int subbook_count = 0;

        EB_Error_Code eb_code =
            eb_subbook_list(&book_, subbook_list, &subbook_count);

        if (eb_code != EB_SUCCESS) {
            error = make_error_code(static_cast<eb_error>(eb_code));
            return false;
        }

        if (subbook_index < subbook_count) {
            eb_code = eb_set_subbook(&book_, subbook_list[subbook_index]);

            if (eb_code == EB_SUCCESS) {
                current_subbook_ = subbook_index;
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

    inline bool SeekText(EB_Position &position, std::error_code &e) {
        if (position.page != last_sought_text_.page ||
            position.offset != last_sought_text_.offset) {
            return SeekEntity(position, e);
        } else {
            return true;
        }
    }

    /**
     * Reads text from the dictionary using supplied libeb function and
     * stores the resulting pointer to a UCS2 string in @text. Memory
     * that's pointed at by the @text should be released using free().
     *
     * Uses javascript callbacks to process EPWING tags. Must be invoked from
     * within a v8 context (Isolate scope, Handle scope, Context scope).
     *
     * Returns the number of characters stored in the @text, or (size_t)-1
     * in case of an error.
     */
    malloc_unique_ptr<uint16_t[]> ReadUcs2Text(ReaderFn function,
                                               EB_Hookset &hookset,
                                               size_t *result_length,
                                               std::error_code &error,
                                               size_t buffer_size_advice = 4096) {

        assert(buffer_size_advice > 0);
        if (result_length)
          *result_length = 0;

        size_t buffer_size = buffer_size_advice * sizeof(uint16_t);
        uint16_t *buffer = reinterpret_cast<uint16_t *>(malloc(buffer_size));
        size_t growth_exponent = 2;
        ssize_t text_length = 0;

        do {
            EB_Error_Code eb_code = (*function)(
                &book_,
                NULL,
                &hookset,
                js_functions_,
                buffer_size * sizeof(uint16_t),
                reinterpret_cast<char *>(buffer),
                &text_length
            );
            if (eb_code != EB_SUCCESS) {
                error = make_error_code(static_cast<eb_error>(eb_code));
                return malloc_unique_ptr<uint16_t[]>(nullptr, ::free);
            }

            buffer_size = buffer_size_advice * sizeof(uint16_t);
            buffer_size *= growth_exponent;
            growth_exponent += 1;

            buffer = reinterpret_cast<uint16_t *>(realloc(buffer, buffer_size));
        } while (!eb_is_text_stopped(&book_));

        if (result_length) {
          assert(text_length >= 0);
          *result_length = static_cast<size_t>(text_length) / sizeof(uint16_t);
        }
        error = make_error_code(simplify_error::success);
        return malloc_unique_ptr<uint16_t[]>(buffer, ::free);
    }

    Likely<std::unique_ptr<char[]>> RefineDictionaryEntry(
                                            JsFunction function,
                                            std::shared_ptr<uint16_t> raw_entry,
                                            size_t entry_length,
                                            size_t *result_length) {
        v8::Isolate *isolate = v8::Isolate::GetCurrent();
        v8::Local<v8::Value> argv[] = {
            v8::String::NewExternalTwoByte(
                isolate,
                new ExternalUcs2String(raw_entry, entry_length)
              ).ToLocalChecked()
        };
        v8::Local<v8::Function> js_callback =
            js_functions_[static_cast<size_t>(function)].Get(isolate);
        // FIXME: use empty object, not undefined.
        v8::MaybeLocal<v8::Value> maybe_result = js_callback->Call(
            isolate->GetCurrentContext(),
            v8::Undefined(isolate),
            sizeof(argv) / sizeof(argv[0]),
            argv
          );

        if (!maybe_result.IsEmpty() &&
            maybe_result.ToLocalChecked()->IsString()) {
          v8::Local<v8::String> result =
              maybe_result.ToLocalChecked().As<v8::String>();
          int length = result->Utf8Length();
          int buffer_size = length + 1;
          char *buffer = new char[buffer_size];

          result->WriteUtf8(
              buffer,
              buffer_size,
              nullptr,
              v8::String::REPLACE_INVALID_UTF8
            );

          if (result_length)
            *result_length = static_cast<size_t>(length);
          return std::unique_ptr<char[]>(buffer);
        } else {
          if (!maybe_result.IsEmpty())
            return make_error_code(simplify_error::unexpected_result_type);
          else
            return make_error_code(simplify_error::js_allocation_error);
        }
    }

    malloc_unique_ptr<uint16_t[]> ReadCurrentEntryTitle(size_t *result_length,
                                                        std::error_code &ec) {
        return ReadUcs2Text(
            &eb_read_heading,
            head_hookset_,
            result_length,
            ec,
            2048
          );
    }

    malloc_unique_ptr<uint16_t[]> ReadCurrentEntryText(size_t *result_length,
                                                       std::error_code &ec) {
        return ReadUcs2Text(
            &eb_read_text,
            text_hookset_,
            result_length,
            ec,
            4096
          );
    }

    inline v8::Local<v8::Context> GetJsContext() {
      return js_context_handle_.Get(isolate_.get());
    }

public:
    EB_Book book_;
    EB_Character_Code charset_;
    EB_Hookset head_hookset_;
    EB_Hookset text_hookset_;
    EB_Position last_sought_text_;

    int current_subbook_;
    std::string script_path_;

    ArrayBufferAllocator array_buffer_allocator_;
    std::unique_ptr<v8::Isolate, std::function<void (v8::Isolate *)>> isolate_;
    v8::Global<v8::Context> js_context_handle_;
    v8::Global<v8::Function> js_functions_[g_js_function_count];
};

class EbSearchResults : public Dictionary::SearchResults {
public:
    EbSearchResults(EpwingDictionary::Private *p,
                    size_t hit_count,
                    malloc_unique_ptr<EB_Hit[]> hits)
      : d(p),
        hit_offset_(static_cast<size_t>(-1)),
        hit_count_(hit_count),
        hits_(std::move(hits))
    {
    }

    virtual ~EbSearchResults() = default;

    size_t GetCount() const override {
        return hit_count_;
    }

    std::error_code SeekNext() override {
        size_t offset = hit_offset_ + 1;
        std::error_code e;

        if (unlikely(offset >= hit_count_))
            return make_error_code(simplify_error::no_more_results);
        else if (unlikely(!d->SeekEntity(hits_[offset].heading, e)))
            return e;

        hit_offset_ = offset;

        // For some reason, searching some dictionaries may return
        // two adjacent results pointing at the same article.
        // It wouldn't be a problem if there were only a few duplicates,
        // but, unfortunately, many searches tend to return more than
        // a few duplicates and it quickly becomes an eyesore.
        if (offset > 0) {
            EB_Position &prevText = hits_[offset - 1].text;
            EB_Position &thisText = hits_[offset].text;

            if (unlikely(prevText.page == thisText.page &&
                         prevText.offset == thisText.offset))
                return SeekNext();
        }

        ENTER_ISOLATE(d->isolate_.get());
        ENTER_CONTEXT(d->GetJsContext());

        malloc_unique_ptr<uint16_t[]> result =
            d->ReadCurrentEntryTitle(&current_entry_length_, e);
        current_entry_text_.reset(result.release(), result.get_deleter());

        return e;
    }

    Likely<std::unique_ptr<char[]>> FetchHeading(size_t *result_size) override {
        return FetchHelper(JsFunction::ProcessHeading, result_size);
    }

    Likely<std::unique_ptr<char[]>> FetchTags(size_t *result_size) override {
        return FetchHelper(JsFunction::ProcessTags, result_size);
    }

    Likely<size_t> FetchGuid(char *buffer, size_t buffer_size) override {
        std::error_code error;
        size_t length = PositionToGuid(hits_[hit_offset_].text,
                                       buffer, buffer_size, error);
        if (length != (size_t) -1)
            return length;
        else
            return error;
    }

private:
    Likely<std::unique_ptr<char[]>> FetchHelper(JsFunction function,
                                                size_t *result_size) {
        ENTER_ISOLATE(d->isolate_.get());
        ENTER_CONTEXT(d->GetJsContext());
        return d->RefineDictionaryEntry(
            function,
            current_entry_text_,
            current_entry_length_,
            result_size
          );
    }

private:
    EpwingDictionary::Private *d;
    size_t hit_offset_;
    size_t hit_count_;
    malloc_unique_ptr<EB_Hit[]> hits_;
    size_t current_entry_length_;
    std::shared_ptr<uint16_t> current_entry_text_;
    v8::Global<v8::Object> current_this_object_;
};

EpwingDictionary::EpwingDictionary(const char *name) : Dictionary(name)
{
    d = new Private();
}

EpwingDictionary::EpwingDictionary(std::string name) : Dictionary(std::move(name))
{
    d = new Private();
}

EpwingDictionary::~EpwingDictionary()
{
    delete d;
}

DictionaryType EpwingDictionary::GetType() const
{
    return DictionaryType::Epwing;
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

        // FIXME: I'm not sure which encoding do subbook titles use.
        // Assuming EUC-JP.
        std::error_code error;
        size_t length = ConvertEucJpToUtf8(buffer, strlen(buffer),
                                           utf8buffer, sizeof(utf8buffer),
                                           error);
        if (!error)
            names_list.push_back(std::string(utf8buffer, length));
        else
            names_list.push_back("<EUC-JP to UTF-8 conversion error>");
    }

    return names_list;
}

std::error_code EpwingDictionary::SelectSubBook(int subbook_index) {
    std::error_code error;

    if (d->SelectSubBook(subbook_index, error)) {
        // Currently selected sub-book is part of permanent state, so save it.
        this->SaveState();
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
    std::unique_ptr<char[]> conv_expr;
    std::error_code last_error;

    // FIXME: what?
    if (d->charset_ != EB_CHARCODE_ISO8859_1) {
        size_t buffer_size = expr_length * 3 + 1;
        conv_expr.reset(new char[buffer_size]);

        ConvertUtf8ToEucJp(
            expr, expr_length + 1,
            conv_expr.get(), buffer_size,
            last_error
          );
    } else {
        size_t buffer_size = expr_length + 1;
        conv_expr.reset(new char[buffer_size]);
        ConvertUtf8ToIso8859_1(
            expr, expr_length + 1,
            conv_expr.get(), buffer_size,
            last_error
          );
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
    EB_Error_Code eb_code = (*search_fun)(&d->book_, conv_expr.get());
    if (eb_code != EB_SUCCESS)
        return make_error_code(static_cast<eb_error>(eb_code));

    return GetResults(limit);
}

Likely<std::unique_ptr<char[]>> EpwingDictionary::ReadText(const char *guid,
                                                           size_t *text_length)
{
    EB_Position position;
    std::error_code ec;

    if (!GuidToPosition(guid, position, ec) || !d->SeekText(position, ec))
        return ec;

    ENTER_ISOLATE(d->isolate_.get());
    ENTER_CONTEXT(d->GetJsContext());

    size_t entry_length = 0;
    malloc_unique_ptr<uint16_t[]> entry_text =
        d->ReadCurrentEntryText(&entry_length, ec);

    if (entry_text) {
      std::shared_ptr<uint16_t> shared_text{entry_text.release(),
                                            entry_text.get_deleter()};
      return d->RefineDictionaryEntry(
          JsFunction::ProcessText,
          shared_text,
          entry_length,
          text_length
        );
    } else {
      return ec;
    }
}

Likely<Dictionary::SearchResults *> EpwingDictionary::GetResults(size_t limit)
{
    EB_Hit *hits = nullptr;
    EB_Error_Code eb_code;
    int increase_step = 256;
    int result_count = 0;
    int adjusted_limit;

    if (limit != 0 || limit <= std::numeric_limits<int>::max())
      adjusted_limit = static_cast<int>(limit);
    else
      adjusted_limit = std::numeric_limits<int>::max();

    // Retrieve search results from libeb by sliding through the hit_list.
    //
    // Since there isn't a way to know number of results in advance, we have to
    // grow our result buffer incrementally.
    while (true) {
        int hit_count;
        int step = std::min(adjusted_limit - result_count, increase_step);
        size_t alloc_size = (result_count + step) * sizeof(EB_Hit);

        hits = reinterpret_cast<EB_Hit *>(realloc(hits, alloc_size));
        eb_code = eb_hit_list(&d->book_, step, hits + result_count, &hit_count);

        if (eb_code == EB_SUCCESS) {
            result_count += hit_count;

            // Any number of search results that is lower than *allocation step*
            // indicates that we are done.
            if (hit_count < step || result_count >= adjusted_limit)
                break;
        } else {
            return make_error_code(static_cast<eb_error>(eb_code));
        }
    }

    return new EbSearchResults(d,
                               static_cast<size_t>(result_count),
                               malloc_unique_ptr<EB_Hit[]>(hits, ::free));
}

Likely<EpwingDictionary *> EpwingDictionary::New(const char *name,
                                                 const char *path,
                                                 const char *script_path)
{
    EpwingDictionary *dict = new EpwingDictionary(name);
    std::error_code error = dict->Initialize(path, script_path, nullptr);

    if (!error) {
        return dict;
    } else {
        delete dict;
        return error;
    }
}

Likely<EpwingDictionary *> EpwingDictionary::NewWithState(const char *name,
                                                          const char *path,
                                                          const nlohmann::json &state)
{
    EpwingDictionary *dict = new EpwingDictionary(name);
    std::error_code error = dict->Initialize(path, nullptr, &state);

    if (!error) {
        return dict;
    } else {
        delete dict;
        return error;
    }
}

std::error_code EpwingDictionary::Initialize(const char *dict_path,
                                             const char *script_path,
                                             const nlohmann::json *state)
{
    using json = nlohmann::json;
    std::error_code last_error;

    if (!InitializeLibEb(last_error))
        return last_error;

    if (!d->Bind(dict_path, last_error))
        return last_error;

    if (state != nullptr) {
        if (auto v = (*state)["subbook"]; v.is_number()) {
            auto index = v.get<json::number_integer_t>();

            // Use our private implementation's SelectSubBook() method to
            // bypass saving state (which we are restoring currently).
            if (!d->SelectSubBook(index, last_error))
                return last_error;
        }

        // Use path to custom script from state, but only if it wasn't
        // explicitly provided.
        if (auto v = (*state)["script"]; v.is_string() && !script_path) {
            auto path = v.get_ref<const json::string_t &>();
            last_error = d->PopulateJsContext(path.c_str());
            if (last_error)
                return last_error;
        }
    }

    if (script_path != nullptr) {
        last_error = d->PopulateJsContext(script_path);
        if (last_error)
            return last_error;
    }

    return last_error;
}

static EB_Error_Code WriteJs(EB_Book *book, void *hook_arg, JsFunction function,
                             int argc, v8::Handle<v8::Value> *argv)
{
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    assert(hook_arg != nullptr);
    assert(isolate != nullptr);

    size_t callback_index = static_cast<size_t>(function);
    auto callback_array = reinterpret_cast<v8::Global<v8::Function>*>(hook_arg);
    v8::Local<v8::Function> callback_fn =
        callback_array[callback_index].Get(isolate);

    assert(!callback_fn.IsEmpty() && callback_fn->IsFunction());

    // FIXME: Pass regular object, not Undefined.
    v8::MaybeLocal<v8::Value> maybe_result =
        callback_fn->Call(context, v8::Undefined(isolate), argc, argv);

    if (!maybe_result.IsEmpty() && maybe_result.ToLocalChecked()->IsString()) {
        v8::Local<v8::String> string =
            maybe_result.ToLocalChecked().As<v8::String>();

        // Use stack for storing short strings.
        if (string->Length() <= 4096) {
            uint16_t buffer[4096];
            size_t bytes_written = (size_t) string->Write(buffer);
            return eb_write_text(book, reinterpret_cast<char *>(buffer),
                                 bytes_written * sizeof(uint16_t));
        } else {
            size_t length = static_cast<size_t>(string->Length());
            std::unique_ptr<uint16_t[]> buffer(new uint16_t[length + 1]);
            size_t bytes_written = (size_t) string->Write(buffer.get());
            return eb_write_text(book, reinterpret_cast<char *>(buffer.get()),
                                 bytes_written * sizeof(uint16_t));
        }
    } else {
        // TODO: handle errors.
        return EB_SUCCESS;
    }
}

inline static EB_Error_Code WriteJs(EB_Book *book, void *hook_arg,
                                    JsFunction function)
{
    return WriteJs(book, hook_arg, function, 0, NULL);
}

static EB_Error_Code HandleIso8859_1(EB_Book *book, EB_Appendix *, void *arg,
                                     EB_Hook_Code, int argc,
                                     const unsigned int *argv)
{
    char ucs2[2] = { (char)argv[0], 0 };
    return eb_write_text(book, ucs2, sizeof(ucs2));
}

static EB_Error_Code HandleJisX0208(EB_Book *book, EB_Appendix *, void *arg,
                                    EB_Hook_Code, int argc,
                                    const unsigned int *argv)
{
    unsigned int c = argv[0];
    size_t size = ((c & 0xff000000) != 0) + ((c & 0x00ff0000) != 0) + \
                  ((c & 0x0000ff00) != 0) + ((c & 0x000000ff) != 0);

    switch (size) {
    case 2: {
        unsigned int hi = (c & 0xff00) >> 8;
        unsigned int lo = c & 0x00ff;
        ConversionEntry *range;

        if (hi >= 0xa1 && hi <= 0xfe && lo >= 0xa1 && lo <= 0xfe)
            range = &g_eucjp_to_ucs2_codeset1_ranges[hi - 0xa1][lo - 0xa1];
        else if (hi == 0x8e && lo >= 0xa1 && lo <= 0xdf)
            range = &g_eucjp_to_ucs2_codeset2[lo - 0xa1];
        else
            return eb_write_text(book, "[?]", 3);

        if (range->ucs2 != NULL)
            return eb_write_text(book, range->ucs2, range->ucs2_length);
        else
            return eb_write_text(book, "[?]", 3);
    }
    case 1: {
        char ucs2[2] = { (char)c, 0 };
        return eb_write_text(book, ucs2, sizeof(ucs2));
    }
    default:
        return eb_write_text(book, "[?]", 3);
    }
}

static EB_Error_Code HandleGb2312(EB_Book *book, EB_Appendix *, void *arg,
                                  EB_Hook_Code, int /*argc*/,
                                  const unsigned int * /*argv*/)
{
    printf("HandleGb2312() not implemented.\n");
    return EB_SUCCESS;
}

static EB_Error_Code HandleBeginSub(EB_Book *book, EB_Appendix *, void *arg,
                                    EB_Hook_Code, int /*argc*/,
                                    const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::BeginSubscript);
}

static EB_Error_Code HandleEndSub(EB_Book *book, EB_Appendix *, void *arg,
                                  EB_Hook_Code, int /*argc*/,
                                  const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::EndSubscript);
}

static EB_Error_Code HandleBeginSup(EB_Book *book, EB_Appendix *, void *arg,
                                    EB_Hook_Code, int /*argc*/,
                                    const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::BeginSuperscript);
}

static EB_Error_Code HandleEndSup(EB_Book *book, EB_Appendix *, void *arg,
                                  EB_Hook_Code, int /*argc*/,
                                  const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::EndSuperscript);
}

static EB_Error_Code HandleIndent(EB_Book *book, EB_Appendix *, void *arg,
                                  EB_Hook_Code, int argc,
                                  const unsigned int *argv)
{
    v8::Local<v8::Value> v8argv[] = {
      v8::Uint32::NewFromUnsigned(v8::Isolate::GetCurrent(), argv[1])
    };
    return WriteJs(book, arg, JsFunction::Indent,
                   sizeof(v8argv) / sizeof(v8argv[0]), v8argv);
}

static EB_Error_Code HandleNewline(EB_Book *book, EB_Appendix *, void *arg,
                                   EB_Hook_Code, int /*argc*/,
                                   const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::Newline);
}

static EB_Error_Code HandleBeginNoBr(EB_Book *book, EB_Appendix *, void *arg,
                                     EB_Hook_Code, int /*argc*/,
                                     const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::BeginNoBreak);
}

static EB_Error_Code HandleEndNoBr(EB_Book *book, EB_Appendix *, void *arg,
                                   EB_Hook_Code, int /*argc*/,
                                   const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::EndNoBreak);
}

static EB_Error_Code HandleBeginEm(EB_Book *book, EB_Appendix *, void *arg,
                                   EB_Hook_Code, int /*argc*/,
                                   const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::BeginEmphasis);
}

static EB_Error_Code HandleEndEm(EB_Book *book, EB_Appendix *, void *arg,
                                 EB_Hook_Code, int /*argc*/,
                                 const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::EndEmphasis);
}

static EB_Error_Code HandleBeginReference(EB_Book *book, EB_Appendix *,
                                          void *arg, EB_Hook_Code, int /*argc*/,
                                          const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::BeginReference);
}

static EB_Error_Code HandleEndReference(EB_Book *book, EB_Appendix *,
                                        void *arg, EB_Hook_Code, int argc,
                                        const unsigned int *argv)
{
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Value> v8argv[] = {
        v8::Uint32::NewFromUnsigned(isolate, argv[1]),
        v8::Uint32::NewFromUnsigned(isolate, argv[2])
    };

    return WriteJs(book, arg, JsFunction::EndReference,
                   sizeof(v8argv) / sizeof(v8argv[0]), v8argv);
}

static EB_Error_Code HandleBeginKeyword(EB_Book *book, EB_Appendix *,
                                        void *arg, EB_Hook_Code, int /*argc*/,
                                        const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::BeginKeyword);
}

static EB_Error_Code HandleEndKeyword(EB_Book *book, EB_Appendix *, void *arg,
                                      EB_Hook_Code, int /*argc*/,
                                      const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::EndKeyword);
}

static EB_Error_Code HandleBeginDecoration(EB_Book *book, EB_Appendix *,
                                           void *arg, EB_Hook_Code, int /*argc*/,
                                           const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::BeginDecoration);
}

static EB_Error_Code HandleEndDecoration(EB_Book *book, EB_Appendix *,
                                         void *arg, EB_Hook_Code, int /*argc*/,
                                         const unsigned int * /*argv*/)
{
    return WriteJs(book, arg, JsFunction::EndDecoration);
}

static EB_Error_Code HandleInsertHGaiji(EB_Book *book, EB_Appendix *,
                                        void *arg, EB_Hook_Code, int argc,
                                        const unsigned int *argv)
{
    v8::Local<v8::Value> v8argv[] = {
        v8::Uint32::NewFromUnsigned(v8::Isolate::GetCurrent(), argv[0])
    };

    return WriteJs(book, arg, JsFunction::InsertHeadingGaiji,
                   sizeof(v8argv) / sizeof(v8argv[0]), v8argv);
}

static EB_Error_Code HandleInsertTGaiji(EB_Book *book, EB_Appendix *,
                                        void *arg, EB_Hook_Code, int argc,
                                        const unsigned int *argv)
{
    v8::Local<v8::Value> v8argv[] = {
        v8::Uint32::NewFromUnsigned(v8::Isolate::GetCurrent(), argv[0])
    };

    return WriteJs(book, arg, JsFunction::InsertTextGaiji,
                   sizeof(v8argv) / sizeof(v8argv[0]), v8argv);
}

void to_json(nlohmann::json &dst, const EpwingDictionary *dict)
{
    dst = nlohmann::json{
        {"subbook", dict->d->current_subbook_},
        {"script", dict->d->script_path_}
    };
}

void to_json(nlohmann::json &dst, const EpwingDictionary &dict)
{
    to_json(dst, &dict);
}

}  // namespace simplify
