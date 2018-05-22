#include <v8.h>
#include "hook.hh"

namespace simplify {

inline static EB_Error_Code WriteByteRange(EB_Book *book, HookContext *context,
                                           Charset charset,
                                           const char *string, size_t length)
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

inline static EB_Error_Code WriteJs(EB_Book *book, HookContext *context,
                                    JsFunction function,
                                    int argc, v8::Handle<v8::Value> *argv)
{
    size_t fn_index = static_cast<size_t>(function);

    assert(!context->js_functions[fn_index].IsEmpty() &&
           context->js_functions[fn_index]->IsFunction());

    using namespace v8;
    Handle<Value> result =
        context->js_functions[fn_index]->Call(Context::GetEntered()->Global(),
                                              argc, argv);

    if (!result.IsEmpty() && result->IsString()) {
        Handle<String> string = result.As<String>();
        uint16_t chars[string->Length() + 1];
        size_t copied = string->Write(chars);

        // Even though the v8 documentation states that the value returned
        // from the String::Write() method is the number of bytes written,
        // in reality though, it's the number of characters written.
        return WriteByteRange(book, context, Charset::Ucs2,
                              reinterpret_cast<char *>(chars),
                              copied * sizeof(uint16_t));
    } else {
        return EB_SUCCESS;
    }
}

inline static EB_Error_Code WriteJs(EB_Book *book, HookContext *context,
                                    JsFunction function)
{
    return WriteJs(book, context, function, 0, NULL);
}

static EB_Error_Code HandleIso8859_1(EB_Book *book, EB_Appendix *, void *arg,
                                     EB_Hook_Code,int argc,
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
                   JsFunction::EndReference,
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
